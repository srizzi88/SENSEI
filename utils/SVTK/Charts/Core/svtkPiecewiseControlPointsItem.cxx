/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPiecewiseControlPointsItem.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPoints2D.h"

// to handle mouse.GetButton
#include "svtkContextMouseEvent.h"

#include <algorithm>
#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPiecewiseControlPointsItem);

//-----------------------------------------------------------------------------
svtkPiecewiseControlPointsItem::svtkPiecewiseControlPointsItem()
{
  this->PiecewiseFunction = nullptr;
}

//-----------------------------------------------------------------------------
svtkPiecewiseControlPointsItem::~svtkPiecewiseControlPointsItem()
{
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
    this->PiecewiseFunction->Delete();
    this->PiecewiseFunction = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
  {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::emitEvent(unsigned long event, void* params)
{
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->InvokeEvent(event, params);
  }
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkPiecewiseControlPointsItem::GetControlPointsMTime()
{
  if (this->PiecewiseFunction)
  {
    return this->PiecewiseFunction->GetMTime();
  }
  return this->GetMTime();
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::SetPiecewiseFunction(svtkPiecewiseFunction* t)
{
  if (t == this->PiecewiseFunction)
  {
    return;
  }
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(PiecewiseFunction, svtkPiecewiseFunction, t);
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->AddObserver(svtkCommand::StartEvent, this->Callback);
    this->PiecewiseFunction->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
    this->PiecewiseFunction->AddObserver(svtkCommand::EndEvent, this->Callback);
  }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
svtkIdType svtkPiecewiseControlPointsItem::GetNumberOfPoints() const
{
  return this->PiecewiseFunction ? static_cast<svtkIdType>(this->PiecewiseFunction->GetSize()) : 0;
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::GetControlPoint(svtkIdType index, double* pos) const
{
  const_cast<svtkPiecewiseFunction*>(this->PiecewiseFunction)->GetNodeValue(index, pos);
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::SetControlPoint(svtkIdType index, double* newPos)
{
  double oldPos[4];
  this->PiecewiseFunction->GetNodeValue(index, oldPos);
  if (newPos[0] != oldPos[0] || newPos[1] != oldPos[1] || newPos[2] != oldPos[2])
  {
    this->StartChanges();
    this->PiecewiseFunction->SetNodeValue(index, newPos);
    this->EndChanges();
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewiseControlPointsItem::EditPoint(float tX, float tY)
{
  if (!this->PiecewiseFunction)
  {
    return;
  }

  this->StartChanges();

  double xvms[4];
  this->PiecewiseFunction->GetNodeValue(this->CurrentPoint, xvms);
  xvms[2] += tX;
  xvms[3] += tY;
  this->PiecewiseFunction->SetNodeValue(this->CurrentPoint, xvms);
  if (this->CurrentPoint > 0)
  {
    this->PiecewiseFunction->GetNodeValue(this->CurrentPoint - 1, xvms);
    xvms[2] += tX;
    xvms[3] += tY;
    this->PiecewiseFunction->SetNodeValue(this->CurrentPoint - 1, xvms);
  }

  this->EndChanges();
}

//-----------------------------------------------------------------------------
svtkIdType svtkPiecewiseControlPointsItem::AddPoint(double* newPos)
{
  if (!this->PiecewiseFunction)
  {
    return -1;
  }

  this->StartChanges();
  svtkIdType addedPoint = this->PiecewiseFunction->AddPoint(newPos[0], newPos[1]);
  this->Superclass::AddPointId(addedPoint);
  this->EndChanges();

  return addedPoint;
}

//-----------------------------------------------------------------------------
svtkIdType svtkPiecewiseControlPointsItem::RemovePoint(double* currentPoint)
{
  if (!this->PiecewiseFunction)
  {
    return -1;
  }
  if (!this->IsPointRemovable(this->GetControlPointId(currentPoint)))
  {
    return -1;
  }

  this->StartChanges();

#ifndef NDEBUG
  svtkIdType expectedPoint =
#endif
    this->svtkControlPointsItem::RemovePoint(currentPoint);
  svtkIdType removedPoint = this->PiecewiseFunction->RemovePoint(currentPoint[0]);
  assert(removedPoint == expectedPoint);

  this->EndChanges();

  return removedPoint;
}
