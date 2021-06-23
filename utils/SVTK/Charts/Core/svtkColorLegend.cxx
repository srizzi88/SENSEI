/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkColorLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkColorLegend.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkScalarsToColors.h"
#include "svtkSmartPointer.h"
#include "svtkTransform2D.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkColorLegend);

//-----------------------------------------------------------------------------
svtkColorLegend::svtkColorLegend()
{
  this->Interpolate = true;
  this->Axis = svtkSmartPointer<svtkAxis>::New();
  this->Axis->SetPosition(svtkAxis::RIGHT);
  this->AddItem(this->Axis);
  this->SetInline(false);
  this->SetHorizontalAlignment(svtkChartLegend::RIGHT);
  this->SetVerticalAlignment(svtkChartLegend::BOTTOM);

  this->Callback = svtkSmartPointer<svtkCallbackCommand>::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(svtkColorLegend::OnScalarsToColorsModified);

  this->TransferFunction = nullptr;

  this->Orientation = svtkColorLegend::VERTICAL;

  this->Position.Set(0.0, 0.0, 0.0, 0.0);
  this->CustomPositionSet = false;
  this->DrawBorder = false;
}

//-----------------------------------------------------------------------------
svtkColorLegend::~svtkColorLegend() = default;

//-----------------------------------------------------------------------------
void svtkColorLegend::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interpolate: " << this->Interpolate << endl;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::GetBounds(double bounds[4])
{
  if (this->TransferFunction)
  {
    bounds[0] = this->TransferFunction->GetRange()[0];
    bounds[1] = this->TransferFunction->GetRange()[1];
  }
  else
  {
    bounds[0] = 0.0;
    bounds[1] = 1.0;
  }
  bounds[2] = 0.0;
  bounds[3] = 1.0;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::Update()
{
  if (this->ImageData == nullptr || this->ImageData->GetMTime() < this->GetMTime())
  {
    this->ComputeTexture();
  }

  // check if the range of our TransferFunction changed
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1])
  {
    svtkWarningMacro(<< "The color transfer function seems to be empty.");
    this->Axis->Update();
    return;
  }

  double axisBounds[2];
  this->Axis->GetUnscaledRange(axisBounds);
  if (bounds[0] != axisBounds[0] || bounds[1] != axisBounds[1])
  {
    this->Axis->SetUnscaledRange(bounds[0], bounds[1]);
  }

  this->Axis->Update();
}

//-----------------------------------------------------------------------------
bool svtkColorLegend::Paint(svtkContext2D* painter)
{
  if (this->TransferFunction == nullptr)
  {
    return true;
  }

  this->GetBoundingRect(painter);

  if (this->DrawBorder)
  {
    // Draw a box around the legend.
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->DrawRect(
      this->Rect.GetX(), this->Rect.GetY(), this->Rect.GetWidth(), this->Rect.GetHeight());
  }

  painter->DrawImage(this->Position, this->ImageData);

  this->Axis->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetTransferFunction(svtkScalarsToColors* transfer)
{
  this->TransferFunction = transfer;
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkColorLegend::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetPoint(float x, float y)
{
  this->Superclass::SetPoint(x, y);
  this->CustomPositionSet = false;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetTextureSize(float w, float h)
{
  this->Position.SetWidth(w);
  this->Position.SetHeight(h);
  this->CustomPositionSet = false;
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetPosition(const svtkRectf& pos)
{
  this->Position = pos;
  this->SetPoint(pos[0], pos[1]);
  this->UpdateAxisPosition();
  this->CustomPositionSet = true;
}

//-----------------------------------------------------------------------------
svtkRectf svtkColorLegend::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
svtkRectf svtkColorLegend::GetBoundingRect(svtkContext2D* painter)
{
  if (this->CacheBounds && this->RectTime > this->GetMTime() && this->RectTime > this->PlotTime &&
    this->RectTime > this->Axis->GetMTime())
  {
    return this->Rect;
  }

  if (!this->CustomPositionSet)
  {
    // if the Position ivar was not explicitly set, we compute the
    // location of the lower left point of the legend here.
    float posX = floor(this->Point[0]);
    float posY = floor(this->Point[1]);
    float posW = this->Position.GetWidth();
    float posH = this->Position.GetHeight();

    if (this->Orientation == svtkColorLegend::VERTICAL)
    {
      // For vertical orientation, we need to move our anchor point
      // further to the left to accommodate the width of the axis.
      // To do this, we query our axis to get its preliminary bounds.
      // Even though its position has not yet been set, its width &
      // height should still be accurate.
      this->UpdateAxisPosition();
      this->Axis->Update();
      svtkRectf axisRect = this->Axis->GetBoundingRect(painter);
      posX -= axisRect.GetWidth();
    }

    // Compute bottom left point based on current alignment.
    if (this->HorizontalAlignment == svtkChartLegend::CENTER)
    {
      posX -= posW / 2.0;
    }
    else if (this->HorizontalAlignment == svtkChartLegend::RIGHT)
    {
      posX -= posW;
    }
    if (this->VerticalAlignment == svtkChartLegend::CENTER)
    {
      posY -= posH / 2.0;
    }
    else if (this->VerticalAlignment == svtkChartLegend::TOP)
    {
      posY -= posH;
    }

    this->Position.SetX(posX);
    this->Position.SetY(posY);
    this->UpdateAxisPosition();
  }

  this->Axis->Update();
  svtkRectf axisRect = this->Axis->GetBoundingRect(painter);

  if (this->Orientation == svtkColorLegend::HORIZONTAL)
  {
    // "+ 1" so the texture doesn't obscure the border
    this->Rect = svtkRectf(this->Position.GetX(), this->Position.GetY() - axisRect.GetHeight() + 1,
      this->Position.GetWidth() + 1, this->Position.GetHeight() + axisRect.GetHeight());
  }
  else
  {
    this->Rect = svtkRectf(this->Position.GetX(), this->Position.GetY(),
      this->Position.GetWidth() + axisRect.GetWidth(), this->Position.GetHeight());
  }

  this->RectTime.Modified();
  return this->Rect;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::ComputeTexture()
{
  if (this->TransferFunction == nullptr)
  {
    return;
  }

  if (!this->ImageData)
  {
    this->ImageData = svtkSmartPointer<svtkImageData>::New();
  }
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1])
  {
    svtkWarningMacro(<< "The color transfer function seems to be empty.");
    return;
  }

  // Set the axis up
  this->Axis->SetUnscaledRange(bounds[0], bounds[1]);
  // this->Axis->AutoScale();

  // Could depend on the screen resolution
  const int dimension = 256;
  double* values = new double[dimension];
  // Texture 1D
  if (this->Orientation == svtkColorLegend::VERTICAL)
  {
    this->ImageData->SetExtent(0, 0, 0, dimension - 1, 0, 0);
  }
  else
  {
    this->ImageData->SetExtent(0, dimension - 1, 0, 0, 0, 0);
  }
  this->ImageData->AllocateScalars(SVTK_UNSIGNED_CHAR, 3);

  for (int i = 0; i < dimension; ++i)
  {
    values[i] = bounds[0] + i * (bounds[1] - bounds[0]) / (dimension - 1);
  }
  unsigned char* ptr = reinterpret_cast<unsigned char*>(this->ImageData->GetScalarPointer());
  this->TransferFunction->MapScalarsThroughTable2(values, ptr, SVTK_DOUBLE, dimension, 1, 3);
  delete[] values;
}

//-----------------------------------------------------------------------------
void svtkColorLegend::OnScalarsToColorsModified(
  svtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  svtkColorLegend* self = reinterpret_cast<svtkColorLegend*>(clientdata);
  self->ScalarsToColorsModified(caller, eid, calldata);
}

//-----------------------------------------------------------------------------
void svtkColorLegend::ScalarsToColorsModified(
  svtkObject* svtkNotUsed(object), unsigned long svtkNotUsed(eid), void* svtkNotUsed(calldata))
{
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetOrientation(int orientation)
{
  if (orientation < 0 || orientation > 1)
  {
    svtkErrorMacro("Error, invalid orientation value supplied: " << orientation);
    return;
  }
  this->Orientation = orientation;
  if (this->Orientation == svtkColorLegend::HORIZONTAL)
  {
    this->Axis->SetPosition(svtkAxis::BOTTOM);
  }
}

//-----------------------------------------------------------------------------
void svtkColorLegend::SetTitle(const svtkStdString& title)
{
  this->Axis->SetTitle(title);
}

//-----------------------------------------------------------------------------
svtkStdString svtkColorLegend::GetTitle()
{
  return this->Axis->GetTitle();
}

//-----------------------------------------------------------------------------
void svtkColorLegend::UpdateAxisPosition()
{
  if (this->Orientation == svtkColorLegend::VERTICAL)
  {
    this->Axis->SetPoint1(
      svtkVector2f(this->Position.GetX() + this->Position.GetWidth(), this->Position.GetY()));
    this->Axis->SetPoint2(svtkVector2f(this->Position.GetX() + this->Position.GetWidth(),
      this->Position.GetY() + this->Position.GetHeight()));
  }
  else
  {
    this->Axis->SetPoint1(svtkVector2f(this->Position.GetX(), this->Position.GetY()));
    this->Axis->SetPoint2(
      svtkVector2f(this->Position.GetX() + this->Position.GetWidth(), this->Position.GetY()));
  }
}

//-----------------------------------------------------------------------------
bool svtkColorLegend::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  bool retval = this->Superclass::MouseMoveEvent(mouse);
  this->Position[0] = this->Point[0];
  this->Position[1] = this->Point[1];
  this->UpdateAxisPosition();
  return retval;
}
