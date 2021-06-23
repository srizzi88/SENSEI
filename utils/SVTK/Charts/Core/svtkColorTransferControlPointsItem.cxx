/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkColorTransferControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkColorTransferControlPointsItem.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkColorTransferFunction.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"

// to handle mouse.GetButton
#include "svtkContextMouseEvent.h"

#include <algorithm>
#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkColorTransferControlPointsItem);

//-----------------------------------------------------------------------------
svtkColorTransferControlPointsItem::svtkColorTransferControlPointsItem()
{
  this->ColorTransferFunction = nullptr;
  this->ColorFill = false;
}

//-----------------------------------------------------------------------------
svtkColorTransferControlPointsItem::~svtkColorTransferControlPointsItem()
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->RemoveObserver(this->Callback);
    this->ColorTransferFunction->Delete();
    this->ColorTransferFunction = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
  {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::emitEvent(unsigned long event, void* params)
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->InvokeEvent(event, params);
  }
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkColorTransferControlPointsItem::GetControlPointsMTime()
{
  if (this->ColorTransferFunction)
  {
    return this->ColorTransferFunction->GetMTime();
  }
  return this->GetMTime();
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::SetColorTransferFunction(svtkColorTransferFunction* t)
{
  if (t == this->ColorTransferFunction)
  {
    return;
  }
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(ColorTransferFunction, svtkColorTransferFunction, t);
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->AddObserver(svtkCommand::StartEvent, this->Callback);
    this->ColorTransferFunction->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
    this->ColorTransferFunction->AddObserver(svtkCommand::EndEvent, this->Callback);
  }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::DrawPoint(svtkContext2D* painter, svtkIdType index)
{
  assert(index != -1);
  if (this->ColorFill)
  {
    double xrgbms[6];
    this->ColorTransferFunction->GetNodeValue(index, xrgbms);
    painter->GetBrush()->SetColorF(xrgbms[1], xrgbms[2], xrgbms[3], 0.55);
  }
  this->svtkControlPointsItem::DrawPoint(painter, index);
}

//-----------------------------------------------------------------------------
svtkIdType svtkColorTransferControlPointsItem::GetNumberOfPoints() const
{
  return this->ColorTransferFunction
    ? static_cast<svtkIdType>(this->ColorTransferFunction->GetSize())
    : 0;
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::GetControlPoint(svtkIdType index, double* pos) const
{
  double xrgbms[6];
  svtkColorTransferFunction* thisTF =
    const_cast<svtkColorTransferFunction*>(this->ColorTransferFunction);

  if (thisTF)
  {
    thisTF->GetNodeValue(index, xrgbms);
    pos[0] = xrgbms[0];
    pos[1] = 0.5;
    pos[2] = xrgbms[4];
    pos[3] = xrgbms[5];
  }
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::SetControlPoint(svtkIdType index, double* newPos)
{
  double xrgbms[6];
  this->ColorTransferFunction->GetNodeValue(index, xrgbms);
  if (newPos[0] != xrgbms[0] || newPos[2] != xrgbms[1] || newPos[3] != xrgbms[2])
  {
    xrgbms[0] = newPos[0];
    xrgbms[4] = newPos[2];
    xrgbms[5] = newPos[3];
    this->StartChanges();
    this->ColorTransferFunction->SetNodeValue(index, xrgbms);
    this->EndChanges();
  }
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::EditPoint(float tX, float tY)
{
  if (!this->ColorTransferFunction)
  {
    return;
  }

  this->StartChanges();

  double xrgbms[6];
  this->ColorTransferFunction->GetNodeValue(this->CurrentPoint, xrgbms);
  xrgbms[4] += tX;
  xrgbms[5] += tY;
  this->ColorTransferFunction->SetNodeValue(this->CurrentPoint, xrgbms);
  if (this->CurrentPoint > 0)
  {
    this->ColorTransferFunction->GetNodeValue(this->CurrentPoint - 1, xrgbms);
    xrgbms[4] += tX;
    xrgbms[5] += tY;
    this->ColorTransferFunction->SetNodeValue(this->CurrentPoint - 1, xrgbms);
  }

  this->EndChanges();
}

//-----------------------------------------------------------------------------
svtkIdType svtkColorTransferControlPointsItem::AddPoint(double* newPos)
{
  if (!this->ColorTransferFunction)
  {
    return -1;
  }

  this->StartChanges();

  double posX = newPos[0];
  double rgb[3] = { 0., 0., 0. };
  this->ColorTransferFunction->GetColor(posX, rgb);
  svtkIdType addedPoint = this->ColorTransferFunction->AddRGBPoint(posX, rgb[0], rgb[1], rgb[2]);
  this->svtkControlPointsItem::AddPointId(addedPoint);

  this->EndChanges();
  return addedPoint;
}

//-----------------------------------------------------------------------------
svtkIdType svtkColorTransferControlPointsItem::RemovePoint(double* currentPoint)
{
  if (!this->ColorTransferFunction ||
    !this->IsPointRemovable(this->GetControlPointId(currentPoint)))
  {
    return -1;
  }

  this->StartChanges();

#ifndef NDEBUG
  svtkIdType expectedPoint =
#endif
    this->svtkControlPointsItem::RemovePoint(currentPoint);
  svtkIdType removedPoint = this->ColorTransferFunction->RemovePoint(currentPoint[0]);
  assert(removedPoint == expectedPoint);

  this->EndChanges();
  return removedPoint;
}

//-----------------------------------------------------------------------------
void svtkColorTransferControlPointsItem::ComputeBounds(double* bounds)
{
  if (this->ColorTransferFunction)
  {
    this->ColorTransferFunction->GetRange(bounds);
    bounds[2] = 0.5;
    bounds[3] = 0.5;

    this->TransformDataToScreen(bounds[0], bounds[2], bounds[0], bounds[2]);
    this->TransformDataToScreen(bounds[1], bounds[3], bounds[1], bounds[3]);
  }
  else
  {
    this->Superclass::ComputeBounds(bounds);
  }
}
