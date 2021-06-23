/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextArea.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextArea.h"

#include "svtkContext2D.h"
#include "svtkContextClip.h"
#include "svtkContextDevice2D.h"
#include "svtkContextTransform.h"
#include "svtkObjectFactory.h"
#include "svtkPlotGrid.h"

#include <algorithm>
#include <cstdlib>

//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkContextArea);

//------------------------------------------------------------------------------
svtkContextArea::svtkContextArea()
  : Geometry(0, 0, 300, 300)
  , DrawAreaBounds(0, 0, 300, 300)
  , DrawAreaGeometry(0, 0, 300, 300)
  , DrawAreaResizeBehavior(DARB_Expand)
  , FixedAspect(1.f)
  , FixedRect(0, 0, 300, 300)
  , FixedMargins(0)
  , FillViewport(true)
{
  this->Axes[svtkAxis::TOP] = this->TopAxis;
  this->Axes[svtkAxis::BOTTOM] = this->BottomAxis;
  this->Axes[svtkAxis::LEFT] = this->LeftAxis;
  this->Axes[svtkAxis::RIGHT] = this->RightAxis;

  this->Grid->SetXAxis(this->BottomAxis);
  this->Grid->SetYAxis(this->LeftAxis);

  this->Axes[svtkAxis::TOP]->SetPosition(svtkAxis::TOP);
  this->Axes[svtkAxis::BOTTOM]->SetPosition(svtkAxis::BOTTOM);
  this->Axes[svtkAxis::LEFT]->SetPosition(svtkAxis::LEFT);
  this->Axes[svtkAxis::RIGHT]->SetPosition(svtkAxis::RIGHT);

  this->InitializeDrawArea();
}

//------------------------------------------------------------------------------
svtkContextArea::~svtkContextArea() = default;

//------------------------------------------------------------------------------
void svtkContextArea::InitializeDrawArea()
{
  for (int i = 0; i < 4; ++i)
  {
    this->AddItem(this->Axes[i]);
  }

  this->Clip->AddItem(this->Transform);
  this->Clip->AddItem(this->Grid);
  this->AddItem(this->Clip);
}

//------------------------------------------------------------------------------
void svtkContextArea::LayoutAxes(svtkContext2D* painter)
{
  // Shorter names for compact readability:
  svtkRectd& data = this->DrawAreaBounds;
  svtkRecti& draw = this->DrawAreaGeometry;

  this->SetAxisRange(data);
  draw = this->ComputeDrawAreaGeometry(painter);

  // Set axes locations to the most recent draw rect:
  this->TopAxis->SetPoint1(draw.GetTopLeft().Cast<float>());
  this->TopAxis->SetPoint2(draw.GetTopRight().Cast<float>());
  this->BottomAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
  this->BottomAxis->SetPoint2(draw.GetBottomRight().Cast<float>());
  this->LeftAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
  this->LeftAxis->SetPoint2(draw.GetTopLeft().Cast<float>());
  this->RightAxis->SetPoint1(draw.GetBottomRight().Cast<float>());
  this->RightAxis->SetPoint2(draw.GetTopRight().Cast<float>());

  // Regenerate ticks, labels, etc:
  for (int i = 0; i < 4; ++i)
  {
    this->Axes[i]->Update();
  }
}

//------------------------------------------------------------------------------
void svtkContextArea::SetAxisRange(svtkRectd const& data)
{
  // Set the data bounds
  this->TopAxis->SetRange(data.GetLeft(), data.GetRight());
  this->BottomAxis->SetRange(data.GetLeft(), data.GetRight());
  this->LeftAxis->SetRange(data.GetBottom(), data.GetTop());
  this->RightAxis->SetRange(data.GetBottom(), data.GetTop());
}

//------------------------------------------------------------------------------
svtkRecti svtkContextArea::ComputeDrawAreaGeometry(svtkContext2D* p)
{
  switch (this->DrawAreaResizeBehavior)
  {
    case svtkContextArea::DARB_Expand:
      return this->ComputeExpandedDrawAreaGeometry(p);
    case svtkContextArea::DARB_FixedAspect:
      return this->ComputeFixedAspectDrawAreaGeometry(p);
    case svtkContextArea::DARB_FixedRect:
      return this->ComputeFixedRectDrawAreaGeometry(p);
    case svtkContextArea::DARB_FixedMargins:
      return this->ComputeFixedMarginsDrawAreaGeometry(p);
    default:
      svtkErrorMacro("Invalid resize behavior enum value: " << this->DrawAreaResizeBehavior);
      break;
  }

  return svtkRecti();
}

//------------------------------------------------------------------------------
svtkRecti svtkContextArea::ComputeExpandedDrawAreaGeometry(svtkContext2D* painter)
{
  // Shorter names for compact readability:
  svtkRecti& geo = this->Geometry;

  // Set the axes positions. We iterate up to 3 times to converge on the margins.
  svtkRecti draw(this->DrawAreaGeometry); // Start with last attempt
  svtkRecti lastDraw;
  for (int pass = 0; pass < 3; ++pass)
  {
    // Set axes locations to the current draw rect:
    this->TopAxis->SetPoint1(draw.GetTopLeft().Cast<float>());
    this->TopAxis->SetPoint2(draw.GetTopRight().Cast<float>());
    this->BottomAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
    this->BottomAxis->SetPoint2(draw.GetBottomRight().Cast<float>());
    this->LeftAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
    this->LeftAxis->SetPoint2(draw.GetTopLeft().Cast<float>());
    this->RightAxis->SetPoint1(draw.GetBottomRight().Cast<float>());
    this->RightAxis->SetPoint2(draw.GetTopRight().Cast<float>());

    // Calculate axes bounds compute new draw geometry:
    svtkVector2i bottomLeft = draw.GetBottomLeft();
    svtkVector2i topRight = draw.GetTopRight();
    for (int i = 0; i < 4; ++i)
    {
      this->Axes[i]->Update();
      svtkRectf bounds = this->Axes[i]->GetBoundingRect(painter);
      switch (static_cast<svtkAxis::Location>(i))
      {
        case svtkAxis::LEFT:
          bottomLeft.SetX(geo.GetLeft() + static_cast<int>(bounds.GetWidth()));
          break;
        case svtkAxis::BOTTOM:
          bottomLeft.SetY(geo.GetBottom() + static_cast<int>(bounds.GetHeight()));
          break;
        case svtkAxis::RIGHT:
          topRight.SetX(geo.GetRight() - static_cast<int>(bounds.GetWidth()));
          break;
        case svtkAxis::TOP:
          topRight.SetY(geo.GetTop() - static_cast<int>(bounds.GetHeight()));
          break;
        case svtkAxis::PARALLEL:
        default:
          abort(); // Shouldn't happen unless svtkAxis::Location is changed.
      }
    }

    // Update draw geometry:
    lastDraw = draw;
    draw.Set(bottomLeft.GetX(), bottomLeft.GetY(), topRight.GetX() - bottomLeft.GetX(),
      topRight.GetY() - bottomLeft.GetY());
    if (draw == lastDraw)
    {
      break; // converged
    }
  }

  return draw;
}

//------------------------------------------------------------------------------
svtkRecti svtkContextArea::ComputeFixedAspectDrawAreaGeometry(svtkContext2D* p)
{
  svtkRecti draw = this->ComputeExpandedDrawAreaGeometry(p);
  float aspect = draw.GetWidth() / static_cast<float>(draw.GetHeight());

  if (aspect > this->FixedAspect) // Too wide:
  {
    int targetWidth = svtkContext2D::FloatToInt(this->FixedAspect * draw.GetHeight());
    int delta = draw.GetWidth() - targetWidth;
    draw.SetX(draw.GetX() + (delta / 2));
    draw.SetWidth(targetWidth);
  }
  else if (aspect < this->FixedAspect) // Too tall:
  {
    int targetHeight = svtkContext2D::FloatToInt(draw.GetWidth() / this->FixedAspect);
    int delta = draw.GetHeight() - targetHeight;
    draw.SetY(draw.GetY() + (delta / 2));
    draw.SetHeight(targetHeight);
  }

  return draw;
}

//------------------------------------------------------------------------------
svtkRecti svtkContextArea::ComputeFixedRectDrawAreaGeometry(svtkContext2D*)
{
  return this->FixedRect;
}

//------------------------------------------------------------------------------
svtkRecti svtkContextArea::ComputeFixedMarginsDrawAreaGeometry(svtkContext2D*)
{
  return svtkRecti(this->FixedMargins[0], this->FixedMargins[2],
    this->Geometry.GetWidth() - (this->FixedMargins[0] + this->FixedMargins[1]),
    this->Geometry.GetHeight() - (this->FixedMargins[2] + this->FixedMargins[3]));
}

//------------------------------------------------------------------------------
void svtkContextArea::UpdateDrawArea()
{
  // Shorter names for compact readability:
  svtkRecti& draw = this->DrawAreaGeometry;

  // Setup clipping:
  this->Clip->SetClip(static_cast<float>(draw.GetX()), static_cast<float>(draw.GetY()),
    static_cast<float>(draw.GetWidth()), static_cast<float>(draw.GetHeight()));

  this->ComputeViewTransform();
}

//------------------------------------------------------------------------------
void svtkContextArea::ComputeViewTransform()
{
  svtkRectd const& data = this->DrawAreaBounds;
  svtkRecti const& draw = this->DrawAreaGeometry;

  this->Transform->Identity();
  this->Transform->Translate(draw.GetX(), draw.GetY());
  this->Transform->Scale(draw.GetWidth() / data.GetWidth(), draw.GetHeight() / data.GetHeight());
  this->Transform->Translate(-data.GetX(), -data.GetY());
}

//------------------------------------------------------------------------------
void svtkContextArea::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

#define svtkContextAreaPrintMemberObject(name)                                                      \
  os << indent << #name ":\n";                                                                     \
  this->name->PrintSelf(os, indent.GetNextIndent())
#define svtkContextAreaPrintMemberPOD(name) os << indent << #name ": " << this->name << "\n"

  svtkContextAreaPrintMemberObject(TopAxis);
  svtkContextAreaPrintMemberObject(BottomAxis);
  svtkContextAreaPrintMemberObject(LeftAxis);
  svtkContextAreaPrintMemberObject(RightAxis);
  svtkContextAreaPrintMemberObject(Grid);
  svtkContextAreaPrintMemberObject(Transform);
  svtkContextAreaPrintMemberPOD(Geometry);
  svtkContextAreaPrintMemberPOD(DrawAreaBounds);
  svtkContextAreaPrintMemberPOD(DrawAreaGeometry);
  os << indent << "DrawAreaResizeBehavior: ";
  switch (this->DrawAreaResizeBehavior)
  {
    case svtkContextArea::DARB_Expand:
      os << "DARB_Expand\n";
      break;
    case svtkContextArea::DARB_FixedAspect:
      os << "DARB_FixedAspect\n";
      break;
    case svtkContextArea::DARB_FixedRect:
      os << "DARB_FixedRect\n";
      break;
    case svtkContextArea::DARB_FixedMargins:
      os << "DARB_FixedMargins\n";
      break;
    default:
      os << "(Invalid enum value: " << this->DrawAreaResizeBehavior << ")\n";
      break;
  }
  svtkContextAreaPrintMemberPOD(FixedAspect);
  svtkContextAreaPrintMemberPOD(FixedRect);
  svtkContextAreaPrintMemberPOD(FixedMargins);
  svtkContextAreaPrintMemberPOD(FillViewport);

#undef svtkContextAreaPrintMemberPOD
#undef svtkContextAreaPrintMemberObject
}

//------------------------------------------------------------------------------
svtkAxis* svtkContextArea::GetAxis(svtkAxis::Location location)
{
  return location < 4 ? this->Axes[location] : nullptr;
}

//------------------------------------------------------------------------------
svtkAbstractContextItem* svtkContextArea::GetDrawAreaItem()
{
  return this->Transform;
}

//------------------------------------------------------------------------------
bool svtkContextArea::Paint(svtkContext2D* painter)
{
  if (this->FillViewport)
  {
    svtkVector2i vpSize = painter->GetDevice()->GetViewportSize();
    this->SetGeometry(svtkRecti(0, 0, vpSize[0], vpSize[1]));
  }

  this->LayoutAxes(painter);
  this->UpdateDrawArea();
  return this->Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedAspect(float aspect)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedAspect);
  if (this->FixedAspect != aspect)
  {
    this->FixedAspect = aspect;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedRect(svtkRecti rect)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedRect);
  if (this->FixedRect != rect)
  {
    this->FixedRect = rect;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedRect(int x, int y, int width, int height)
{
  this->SetFixedRect(svtkRecti(x, y, width, height));
}

//------------------------------------------------------------------------------
void svtkContextArea::GetFixedMarginsArray(int margins[4])
{
  std::copy(this->FixedMargins.GetData(), this->FixedMargins.GetData() + 4, margins);
}

//------------------------------------------------------------------------------
const int* svtkContextArea::GetFixedMarginsArray()
{
  return this->FixedMargins.GetData();
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedMargins(svtkContextArea::Margins margins)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedMargins);
  if (margins != this->FixedMargins)
  {
    this->FixedMargins = margins;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedMargins(int margins[4])
{
  this->SetFixedMargins(Margins(margins));
}

//------------------------------------------------------------------------------
void svtkContextArea::SetFixedMargins(int left, int right, int bottom, int top)
{
  Margins margins;
  margins[0] = left;
  margins[1] = right;
  margins[2] = bottom;
  margins[3] = top;
  this->SetFixedMargins(margins);
}

//------------------------------------------------------------------------------
void svtkContextArea::SetShowGrid(bool show)
{
  this->Grid->SetVisible(show);
}

//------------------------------------------------------------------------------
bool svtkContextArea::GetShowGrid()
{
  return this->Grid->GetVisible();
}
