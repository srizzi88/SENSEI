/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBorderRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBorderRepresentation.h"
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkWindow.h"

#include <algorithm>
#include <cassert>

svtkStandardNewMacro(svtkBorderRepresentation);

//-------------------------------------------------------------------------
svtkBorderRepresentation::svtkBorderRepresentation()
{
  this->InteractionState = svtkBorderRepresentation::Outside;

  this->ShowVerticalBorder = BORDER_ON;
  this->ShowHorizontalBorder = BORDER_ON;
  this->ProportionalResize = 0;
  this->Tolerance = 3;
  this->SelectionPoint[0] = this->SelectionPoint[1] = 0.0;

  // Initial positioning information
  this->Negotiated = 0;
  this->PositionCoordinate = svtkCoordinate::New();
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.05, 0.05);
  this->Position2Coordinate = svtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.1, 0.1); // may be updated by the subclass
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);

  // Create the geometry in canonical coordinates
  this->BWPoints = svtkPoints::New();
  this->BWPoints->SetDataTypeToDouble();
  this->BWPoints->SetNumberOfPoints(4);
  this->BWPoints->SetPoint(0, 0.0, 0.0, 0.0); // may be updated by the subclass
  this->BWPoints->SetPoint(1, 1.0, 0.0, 0.0);
  this->BWPoints->SetPoint(2, 1.0, 1.0, 0.0);
  this->BWPoints->SetPoint(3, 0.0, 1.0, 0.0);

  svtkCellArray* outline = svtkCellArray::New();
  outline->InsertNextCell(5);
  outline->InsertCellPoint(0);
  outline->InsertCellPoint(1);
  outline->InsertCellPoint(2);
  outline->InsertCellPoint(3);
  outline->InsertCellPoint(0);

  this->BWPolyData = svtkPolyData::New();
  this->BWPolyData->SetPoints(this->BWPoints);
  this->BWPolyData->SetLines(outline);
  outline->Delete();

  this->BWTransform = svtkTransform::New();
  this->BWTransformFilter = svtkTransformPolyDataFilter::New();
  this->BWTransformFilter->SetTransform(this->BWTransform);
  this->BWTransformFilter->SetInputData(this->BWPolyData);

  this->BWMapper = svtkPolyDataMapper2D::New();
  this->BWMapper->SetInputConnection(this->BWTransformFilter->GetOutputPort());
  this->BWActor = svtkActor2D::New();
  this->BWActor->SetMapper(this->BWMapper);

  this->BorderProperty = svtkProperty2D::New();
  this->BWActor->SetProperty(this->BorderProperty);

  this->MinimumSize[0] = 1;
  this->MinimumSize[1] = 1;
  this->MaximumSize[0] = 100000;
  this->MaximumSize[1] = 100000;

  this->Moving = 0;
}

//-------------------------------------------------------------------------
svtkBorderRepresentation::~svtkBorderRepresentation()
{
  this->PositionCoordinate->Delete();
  this->Position2Coordinate->Delete();

  this->BWPoints->Delete();
  this->BWTransform->Delete();
  this->BWTransformFilter->Delete();
  this->BWPolyData->Delete();
  this->BWMapper->Delete();
  this->BWActor->Delete();
  this->BorderProperty->Delete();
}

//----------------------------------------------------------------------------
svtkMTimeType svtkBorderRepresentation::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->PositionCoordinate->GetMTime());
  mTime = std::max(mTime, this->Position2Coordinate->GetMTime());
  mTime = std::max(mTime, this->BorderProperty->GetMTime());
  return mTime;
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::SetShowBorder(int border)
{
  this->SetShowVerticalBorder(border);
  this->SetShowHorizontalBorder(border);
  this->UpdateShowBorder();
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::GetShowBorderMinValue()
{
  return BORDER_OFF;
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::GetShowBorderMaxValue()
{
  return BORDER_ACTIVE;
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::GetShowBorder()
{
  return this->GetShowVerticalBorder() != BORDER_OFF ? this->GetShowVerticalBorder()
                                                     : this->GetShowHorizontalBorder();
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::StartWidgetInteraction(double eventPos[2])
{
  this->StartEventPosition[0] = eventPos[0];
  this->StartEventPosition[1] = eventPos[1];
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::WidgetInteraction(double eventPos[2])
{
  double XF = eventPos[0];
  double YF = eventPos[1];

  // convert to normalized viewport coordinates
  this->Renderer->DisplayToNormalizedDisplay(XF, YF);
  this->Renderer->NormalizedDisplayToViewport(XF, YF);
  this->Renderer->ViewportToNormalizedViewport(XF, YF);

  // there are four parameters that can be adjusted
  double* fpos1 = this->PositionCoordinate->GetValue();
  double* fpos2 = this->Position2Coordinate->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];

  double delX = XF - this->StartEventPosition[0];
  double delY = YF - this->StartEventPosition[1];
  double delX2 = 0.0, delY2 = 0.0;

  // Based on the state, adjust the representation. Note that we force a
  // uniform scaling of the widget when tugging on the corner points (and
  // when proportional resize is on). This is done by finding the maximum
  // movement in the x-y directions and using this to scale the widget.
  if (this->ProportionalResize && !this->Moving)
  {
    double sx = fpos2[0] / fpos2[1];
    double sy = fpos2[1] / fpos2[0];
    if (fabs(delX) > fabs(delY))
    {
      delY = sy * delX;
      delX2 = delX;
      delY2 = -delY;
    }
    else
    {
      delX = sx * delY;
      delY2 = delY;
      delX2 = -delX;
    }
  }
  else
  {
    delX2 = delX;
    delY2 = delY;
  }

  // The previous "if" statement has taken care of the proportional resize
  // for the most part. However, tugging on edges has special behavior, which
  // is to scale the box about its center.
  switch (this->InteractionState)
  {
    case svtkBorderRepresentation::AdjustingP0:
      par1[0] = par1[0] + delX;
      par1[1] = par1[1] + delY;
      break;
    case svtkBorderRepresentation::AdjustingP1:
      par2[0] = par2[0] + delX2;
      par1[1] = par1[1] + delY2;
      break;
    case svtkBorderRepresentation::AdjustingP2:
      par2[0] = par2[0] + delX;
      par2[1] = par2[1] + delY;
      break;
    case svtkBorderRepresentation::AdjustingP3:
      par1[0] = par1[0] + delX2;
      par2[1] = par2[1] + delY2;
      break;
    case svtkBorderRepresentation::AdjustingE0:
      par1[1] = par1[1] + delY;
      if (this->ProportionalResize)
      {
        par2[1] = par2[1] - delY;
        par1[0] = par1[0] + delX;
        par2[0] = par2[0] - delX;
      }
      break;
    case svtkBorderRepresentation::AdjustingE1:
      par2[0] = par2[0] + delX;
      if (this->ProportionalResize)
      {
        par1[0] = par1[0] - delX;
        par1[1] = par1[1] - delY;
        par2[1] = par2[1] + delY;
      }
      break;
    case svtkBorderRepresentation::AdjustingE2:
      par2[1] = par2[1] + delY;
      if (this->ProportionalResize)
      {
        par1[1] = par1[1] - delY;
        par1[0] = par1[0] - delX;
        par2[0] = par2[0] + delX;
      }
      break;
    case svtkBorderRepresentation::AdjustingE3:
      par1[0] = par1[0] + delX;
      if (this->ProportionalResize)
      {
        par2[0] = par2[0] - delX;
        par1[1] = par1[1] + delY;
        par2[1] = par2[1] - delY;
      }
      break;
    case svtkBorderRepresentation::Inside:
      if (this->Moving)
      {
        par1[0] = par1[0] + delX;
        par1[1] = par1[1] + delY;
        par2[0] = par2[0] + delX;
        par2[1] = par2[1] + delY;
      }
      break;
  }

  // Modify the representation
  if (par2[0] > par1[0] && par2[1] > par1[1])
  {
    this->PositionCoordinate->SetValue(par1[0], par1[1]);
    this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartEventPosition[0] = XF;
    this->StartEventPosition[1] = YF;
  }

  this->Modified();
  this->BuildRepresentation();
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::NegotiateLayout()
{
  double size[2];
  this->GetSize(size);

  // Update the initial border geometry
  this->BWPoints->SetPoint(0, 0.0, 0.0, 0.0); // may be updated by the subclass
  this->BWPoints->SetPoint(1, size[0], 0.0, 0.0);
  this->BWPoints->SetPoint(2, size[0], size[1], 0.0);
  this->BWPoints->SetPoint(3, 0.0, size[1], 0.0);
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::ComputeInteractionState(int X, int Y, int svtkNotUsed(modify))
{
  int* pos1 = this->PositionCoordinate->GetComputedDisplayValue(this->Renderer);
  int* pos2 = this->Position2Coordinate->GetComputedDisplayValue(this->Renderer);

  // Figure out where we are in the widget. Exclude outside case first.
  if (X < (pos1[0] - this->Tolerance) || (pos2[0] + this->Tolerance) < X ||
    Y < (pos1[1] - this->Tolerance) || (pos2[1] + this->Tolerance) < Y)
  {
    this->InteractionState = svtkBorderRepresentation::Outside;
  }

  else // we are on the boundary or inside the border
  {
    // Now check for proximinity to edges and points
    int e0 = (Y >= (pos1[1] - this->Tolerance) && Y <= (pos1[1] + this->Tolerance));
    int e1 = (X >= (pos2[0] - this->Tolerance) && X <= (pos2[0] + this->Tolerance));
    int e2 = (Y >= (pos2[1] - this->Tolerance) && Y <= (pos2[1] + this->Tolerance));
    int e3 = (X >= (pos1[0] - this->Tolerance) && X <= (pos1[0] + this->Tolerance));

    int adjustHorizontalEdges = (this->ShowHorizontalBorder != BORDER_OFF);
    int adjustVerticalEdges = (this->ShowVerticalBorder != BORDER_OFF);
    int adjustPoints = (adjustHorizontalEdges && adjustVerticalEdges);

    if (e0 && e1 && adjustPoints)
    {
      this->InteractionState = svtkBorderRepresentation::AdjustingP1;
    }
    else if (e1 && e2 && adjustPoints)
    {
      this->InteractionState = svtkBorderRepresentation::AdjustingP2;
    }
    else if (e2 && e3 && adjustPoints)
    {
      this->InteractionState = svtkBorderRepresentation::AdjustingP3;
    }
    else if (e3 && e0 && adjustPoints)
    {
      this->InteractionState = svtkBorderRepresentation::AdjustingP0;
    }

    // Edges
    else if (e0 || e1 || e2 || e3)
    {
      if (e0 && adjustHorizontalEdges)
      {
        this->InteractionState = svtkBorderRepresentation::AdjustingE0;
      }
      else if (e1 && adjustVerticalEdges)
      {
        this->InteractionState = svtkBorderRepresentation::AdjustingE1;
      }
      else if (e2 && adjustHorizontalEdges)
      {
        this->InteractionState = svtkBorderRepresentation::AdjustingE2;
      }
      else if (e3 && adjustVerticalEdges)
      {
        this->InteractionState = svtkBorderRepresentation::AdjustingE3;
      }
    }

    else // must be interior
    {
      if (this->Moving)
      {
        // FIXME: This must be wrong.  Moving is not an entry in the
        // _InteractionState enum.  It is an ivar flag and it has no business
        // being set to InteractionState.  This just happens to work because
        // Inside happens to be 1, and this gets set when Moving is 1.
        this->InteractionState = svtkBorderRepresentation::Moving;
      }
      else
      {
        this->InteractionState = svtkBorderRepresentation::Inside;
      }
    }
  } // else inside or on border
  this->UpdateShowBorder();

  return this->InteractionState;
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::UpdateShowBorder()
{
  enum
  {
    NoBorder = 0x00,
    VerticalBorder = 0x01,
    HorizontalBorder = 0x02,
    AllBorders = VerticalBorder | HorizontalBorder
  };
  int currentBorder = NoBorder;
  switch (this->BWPolyData->GetLines()->GetNumberOfCells())
  {
    case 1:
      currentBorder = AllBorders;
      break;
    case 2:
    {
      svtkIdType npts = 0;
      const svtkIdType* pts = nullptr;
      this->BWPolyData->GetLines()->GetCellAtId(0, npts, pts);
      assert(npts == 2);
      currentBorder = (pts[0] == 0 ? HorizontalBorder : VerticalBorder);
      break;
    }
    case 0:
    default: // not supported
      currentBorder = NoBorder;
      break;
  }
  int newBorder = NoBorder;
  if (this->ShowVerticalBorder == this->ShowHorizontalBorder)
  {
    newBorder = (this->ShowVerticalBorder == BORDER_ON ||
                  (this->ShowVerticalBorder == BORDER_ACTIVE &&
                    this->InteractionState != svtkBorderRepresentation::Outside))
      ? AllBorders
      : NoBorder;
  }
  else
  {
    newBorder = newBorder |
      ((this->ShowVerticalBorder == BORDER_ON ||
         (this->ShowVerticalBorder == BORDER_ACTIVE &&
           this->InteractionState != svtkBorderRepresentation::Outside))
          ? VerticalBorder
          : NoBorder);
    newBorder = newBorder |
      ((this->ShowHorizontalBorder == BORDER_ON ||
         (this->ShowHorizontalBorder == BORDER_ACTIVE &&
           this->InteractionState != svtkBorderRepresentation::Outside))
          ? HorizontalBorder
          : NoBorder);
  }
  bool visible = (newBorder != NoBorder);
  if (currentBorder != newBorder && visible)
  {
    svtkCellArray* outline = svtkCellArray::New();
    switch (newBorder)
    {
      case AllBorders:
        outline->InsertNextCell(5);
        outline->InsertCellPoint(0);
        outline->InsertCellPoint(1);
        outline->InsertCellPoint(2);
        outline->InsertCellPoint(3);
        outline->InsertCellPoint(0);
        break;
      case VerticalBorder:
        outline->InsertNextCell(2);
        outline->InsertCellPoint(1);
        outline->InsertCellPoint(2);
        outline->InsertNextCell(2);
        outline->InsertCellPoint(3);
        outline->InsertCellPoint(0);
        break;
      case HorizontalBorder:
        outline->InsertNextCell(2);
        outline->InsertCellPoint(0);
        outline->InsertCellPoint(1);
        outline->InsertNextCell(2);
        outline->InsertCellPoint(2);
        outline->InsertCellPoint(3);
        break;
      default:
        break;
    }
    this->BWPolyData->SetLines(outline);
    outline->Delete();
    this->BWPolyData->Modified();
    this->Modified();
  }
  this->BWActor->SetVisibility(visible);
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::BuildRepresentation()
{
  if (this->Renderer &&
    (this->GetMTime() > this->BuildTime ||
      (this->Renderer->GetSVTKWindow() &&
        this->Renderer->GetSVTKWindow()->GetMTime() > this->BuildTime)))
  {
    // Negotiate with subclasses
    if (!this->Negotiated)
    {
      this->NegotiateLayout();
      this->Negotiated = 1;
    }

    // Set things up
    int* pos1 = this->PositionCoordinate->GetComputedViewportValue(this->Renderer);
    int* pos2 = this->Position2Coordinate->GetComputedViewportValue(this->Renderer);

    // If the widget's aspect ratio is to be preserved (ProportionalResizeOn),
    // then (pos1,pos2) are a bounding rectangle.
    if (this->ProportionalResize)
    {
    }

    // Now transform the canonical widget into display coordinates
    double size[2];
    this->GetSize(size);
    double tx = pos1[0];
    double ty = pos1[1];
    double sx = (pos2[0] - pos1[0]) / size[0];
    double sy = (pos2[1] - pos1[1]) / size[1];

    this->BWTransform->Identity();
    this->BWTransform->Translate(tx, ty, 0.0);
    this->BWTransform->Scale(sx, sy, 1);

    this->BuildTime.Modified();
  }
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::GetActors2D(svtkPropCollection* pc)
{
  pc->AddItem(this->BWActor);
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  this->BWActor->ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::RenderOverlay(svtkViewport* w)
{
  this->BuildRepresentation();
  if (!this->BWActor->GetVisibility())
  {
    return 0;
  }
  return this->BWActor->RenderOverlay(w);
}

//-------------------------------------------------------------------------
int svtkBorderRepresentation::RenderOpaqueGeometry(svtkViewport* w)
{
  this->BuildRepresentation();
  if (!this->BWActor->GetVisibility())
  {
    return 0;
  }
  return this->BWActor->RenderOpaqueGeometry(w);
}

//-----------------------------------------------------------------------------
int svtkBorderRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* w)
{
  this->BuildRepresentation();
  if (!this->BWActor->GetVisibility())
  {
    return 0;
  }
  return this->BWActor->RenderTranslucentPolygonalGeometry(w);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkBorderRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  if (!this->BWActor->GetVisibility())
  {
    return 0;
  }
  return this->BWActor->HasTranslucentPolygonalGeometry();
}

//-------------------------------------------------------------------------
void svtkBorderRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Show Vertical Border: ";
  if (this->ShowVerticalBorder == BORDER_OFF)
  {
    os << "Off\n";
  }
  else if (this->ShowVerticalBorder == BORDER_ON)
  {
    os << "On\n";
  }
  else // if ( this->ShowVerticalBorder == BORDER_ACTIVE)
  {
    os << "Active\n";
  }

  os << indent << "Show Horizontal Border: ";
  if (this->ShowHorizontalBorder == BORDER_OFF)
  {
    os << "Off\n";
  }
  else if (this->ShowHorizontalBorder == BORDER_ON)
  {
    os << "On\n";
  }
  else // if ( this->ShowHorizontalBorder == BORDER_ACTIVE)
  {
    os << "Active\n";
  }

  if (this->BorderProperty)
  {
    os << indent << "Border Property:\n";
    this->BorderProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Border Property: (none)\n";
  }

  os << indent << "Proportional Resize: " << (this->ProportionalResize ? "On\n" : "Off\n");
  os << indent << "Minimum Size: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
  os << indent << "Maximum Size: " << this->MaximumSize[0] << " " << this->MaximumSize[1] << endl;

  os << indent << "Moving: " << (this->Moving ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Selection Point: (" << this->SelectionPoint[0] << "," << this->SelectionPoint[1]
     << "}\n";
}
