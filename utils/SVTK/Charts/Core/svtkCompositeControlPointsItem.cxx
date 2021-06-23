/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCompositeControlPointsItem.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkColorTransferFunction.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkIdTypeArray.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPiecewisePointHandleItem.h"
#include "svtkPoints2D.h"

// to handle mouse.GetButton
#include "svtkContextMouseEvent.h"

#include <algorithm>
#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkCompositeControlPointsItem);

//-----------------------------------------------------------------------------
svtkCompositeControlPointsItem::svtkCompositeControlPointsItem()
{
  this->PointsFunction = ColorAndOpacityPointsFunction;
  this->OpacityFunction = nullptr;
  this->ColorFill = true;
  this->OpacityPointHandle = nullptr;
  this->UseOpacityPointHandles = false;
}

//-----------------------------------------------------------------------------
svtkCompositeControlPointsItem::~svtkCompositeControlPointsItem()
{
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
    this->OpacityFunction->Delete();
    this->OpacityFunction = nullptr;
  }
  if (this->OpacityPointHandle)
  {
    this->OpacityPointHandle->Delete();
    this->OpacityPointHandle = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OpacityFunction: ";
  if (this->OpacityFunction)
  {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "OpacityFunction: ";
  if (this->OpacityPointHandle)
  {
    os << endl;
    this->OpacityPointHandle->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "UseOpacityPointHandles: " << this->UseOpacityPointHandles << endl;
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::emitEvent(unsigned long event, void* params)
{
  if (this->OpacityFunction)
  {
    this->OpacityFunction->InvokeEvent(event, params);
  }
  this->Superclass::emitEvent(event, params);
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkCompositeControlPointsItem::GetControlPointsMTime()
{
  svtkMTimeType mTime = this->Superclass::GetControlPointsMTime();
  if (this->OpacityFunction)
  {
    mTime = std::max(mTime, this->OpacityFunction->GetMTime());
  }
  return mTime;
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::SetOpacityFunction(svtkPiecewiseFunction* function)
{
  if (function == this->OpacityFunction)
  {
    return;
  }
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
  }
  svtkSetObjectBodyMacro(OpacityFunction, svtkPiecewiseFunction, function);
  if (this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    this->SilentMergeTransferFunctions();
  }
  if (this->OpacityFunction)
  {
    this->OpacityFunction->AddObserver(svtkCommand::StartEvent, this->Callback);
    this->OpacityFunction->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
    this->OpacityFunction->AddObserver(svtkCommand::EndEvent, this->Callback);
  }
  this->ResetBounds();
  this->ComputePoints();
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::SetColorTransferFunction(svtkColorTransferFunction* c)
{
  if (c == this->ColorTransferFunction)
  {
    return;
  }
  // We need to set the color transfer function here (before
  // Superclass::SetPiecewiseFunction) to be able to have a valid
  // color transfer function for MergeColorTransferFunction().
  this->Superclass::SetColorTransferFunction(c);
  if (this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    this->SilentMergeTransferFunctions();
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::DrawPoint(svtkContext2D* painter, svtkIdType index)
{
  if (this->PointsFunction == ColorPointsFunction ||
    this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    this->Superclass::DrawPoint(painter, index);
    return;
  }
  if (this->PointsFunction == OpacityPointsFunction && this->ColorFill &&
    this->ColorTransferFunction)
  {
    double xvms[4];
    this->OpacityFunction->GetNodeValue(index, xvms);
    const unsigned char* rgb = this->ColorTransferFunction->MapValue(xvms[0]);
    painter->GetBrush()->SetColorF(rgb[0] / 255., rgb[1] / 255., rgb[2] / 255., 0.55);
  }
  this->svtkControlPointsItem::DrawPoint(painter, index);
}

//-----------------------------------------------------------------------------
svtkIdType svtkCompositeControlPointsItem::GetNumberOfPoints() const
{
  if (this->ColorTransferFunction &&
    (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    return this->Superclass::GetNumberOfPoints();
  }
  if (this->OpacityFunction &&
    (this->PointsFunction == OpacityPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    return static_cast<svtkIdType>(this->OpacityFunction->GetSize());
  }
  return 0;
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::SetControlPoint(svtkIdType index, double* newPos)
{
  if (this->PointsFunction == ColorPointsFunction ||
    this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    this->Superclass::SetControlPoint(index, newPos);
  }
  if (this->OpacityFunction &&
    (this->PointsFunction == OpacityPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    this->StartChanges();
    this->OpacityFunction->SetNodeValue(index, newPos);
    this->EndChanges();
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::GetControlPoint(svtkIdType index, double* pos) const
{
  if (!this->OpacityFunction || this->PointsFunction == ColorPointsFunction)
  {
    this->Superclass::GetControlPoint(index, pos);
    if (this->OpacityFunction)
    {
      pos[1] = const_cast<svtkPiecewiseFunction*>(this->OpacityFunction)->GetValue(pos[0]);
    }
    return;
  }
  const_cast<svtkPiecewiseFunction*>(this->OpacityFunction)->GetNodeValue(index, pos);
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::EditPoint(float tX, float tY)
{
  if (this->PointsFunction == ColorPointsFunction ||
    this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    this->Superclass::EditPoint(tX, tY);
  }
  if (this->OpacityFunction &&
    (this->PointsFunction == ColorPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    this->StartChanges();
    double xvms[4];
    this->OpacityFunction->GetNodeValue(this->CurrentPoint, xvms);
    xvms[2] += tX;
    xvms[3] += tY;
    this->OpacityFunction->SetNodeValue(this->CurrentPoint, xvms);
    // Not sure why we move the point before too
    if (this->CurrentPoint > 0)
    {
      this->OpacityFunction->GetNodeValue(this->CurrentPoint - 1, xvms);
      xvms[2] += tX;
      xvms[3] += tY;
      this->OpacityFunction->SetNodeValue(this->CurrentPoint - 1, xvms);
    }
    this->EndChanges();
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkCompositeControlPointsItem::AddPoint(double* newPos)
{
  svtkIdType addedPoint = -1;
  this->StartChanges();
  if (this->OpacityFunction &&
    (this->PointsFunction == OpacityPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    addedPoint = this->OpacityFunction->AddPoint(newPos[0], newPos[1]);
    if (this->PointsFunction == OpacityPointsFunction)
    {
      this->svtkControlPointsItem::AddPointId(addedPoint);
    }
  }
  if (this->PointsFunction == ColorPointsFunction ||
    this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    addedPoint = this->Superclass::AddPoint(newPos);
  }
  this->EndChanges();
  return addedPoint;
}

//-----------------------------------------------------------------------------
svtkIdType svtkCompositeControlPointsItem::RemovePoint(double* currentPoint)
{
  svtkIdType removedPoint = -1;
  if (!this->IsPointRemovable(this->GetControlPointId(currentPoint)))
  {
    return removedPoint;
  }

  this->StartChanges();
  if (this->PointsFunction == ColorPointsFunction ||
    this->PointsFunction == ColorAndOpacityPointsFunction)
  {
    removedPoint = this->Superclass::RemovePoint(currentPoint);
  }
  if (this->OpacityFunction &&
    (this->PointsFunction == OpacityPointsFunction ||
      this->PointsFunction == ColorAndOpacityPointsFunction))
  {
    removedPoint = this->OpacityFunction->RemovePoint(currentPoint[0]);
  }
  this->EndChanges();
  return removedPoint;
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::MergeTransferFunctions()
{
  if (!this->ColorTransferFunction || !this->OpacityFunction)
  {
    return;
  }
  // Naive implementation that does the work but can be a bit slow
  // Copy OpacityFunction points into the ColorTransferFunction
  const int piecewiseFunctionCount = this->OpacityFunction->GetSize();
  for (int i = 0; i < piecewiseFunctionCount; ++i)
  {
    double piecewisePoint[4];
    this->OpacityFunction->GetNodeValue(i, piecewisePoint);
    double rgb[3];
    this->ColorTransferFunction->GetColor(piecewisePoint[0], rgb);
    // note that we might lose the midpoint/sharpness of the point if any
    this->ColorTransferFunction->RemovePoint(piecewisePoint[0]);
    this->ColorTransferFunction->AddRGBPoint(
      piecewisePoint[0], rgb[0], rgb[1], rgb[2], piecewisePoint[2], piecewisePoint[3]);
  }
  // Copy ColorTransferFunction points into the OpacityFunction
  const int colorFunctionCount = this->ColorTransferFunction->GetSize();
  for (int i = 0; i < colorFunctionCount; ++i)
  {
    double xrgbms[6];
    this->ColorTransferFunction->GetNodeValue(i, xrgbms);
    double value = this->OpacityFunction->GetValue(xrgbms[0]);
    // note that we might lose the midpoint/sharpness of the point if any
    this->OpacityFunction->RemovePoint(xrgbms[0]);
    this->OpacityFunction->AddPoint(xrgbms[0], value, xrgbms[4], xrgbms[5]);
  }
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::SilentMergeTransferFunctions()
{
  this->StartChanges();
  this->MergeTransferFunctions();
  this->EndChanges();
}

//-----------------------------------------------------------------------------
bool svtkCompositeControlPointsItem::MouseButtonPressEvent(const svtkContextMouseEvent& mouse)
{
  bool result = false;
  if (this->OpacityPointHandle && this->OpacityPointHandle->GetVisible())
  {
    result = this->OpacityPointHandle->MouseButtonPressEvent(mouse);
  }
  if (!result)
  {
    result = this->Superclass::MouseButtonPressEvent(mouse);
    if (result && this->OpacityPointHandle && this->OpacityPointHandle->GetVisible() &&
      this->OpacityPointHandle->GetCurrentPointIndex() != this->GetCurrentPoint())
    {
      this->OpacityPointHandle->SetVisible(false);
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
bool svtkCompositeControlPointsItem::MouseDoubleClickEvent(const svtkContextMouseEvent& mouse)
{
  bool superRes = this->Superclass::MouseDoubleClickEvent(mouse);
  if (superRes)
  {
    svtkIdType curIdx = this->GetCurrentPoint();
    this->EditPointCurve(curIdx);
  }
  return superRes;
}

//-----------------------------------------------------------------------------
bool svtkCompositeControlPointsItem::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  bool result = false;
  if (this->OpacityPointHandle && this->OpacityPointHandle->GetVisible())
  {
    result = this->OpacityPointHandle->MouseMoveEvent(mouse);
  }
  if (!result)
  {
    result = this->Superclass::MouseMoveEvent(mouse);
  }
  return result;
}

//-----------------------------------------------------------------------------
void svtkCompositeControlPointsItem::EditPointCurve(svtkIdType index)
{
  if (index < 0 || index >= this->GetNumberOfPoints())
  {
    return;
  }
  if (this->GetUseOpacityPointHandles())
  {
    if (!this->OpacityPointHandle)
    {
      this->OpacityPointHandle = svtkPiecewisePointHandleItem::New();
      this->AddItem(this->OpacityPointHandle);
      this->OpacityPointHandle->SetPiecewiseFunction(this->GetOpacityFunction());
    }
    else
    {
      this->OpacityPointHandle->SetVisible(!this->OpacityPointHandle->GetVisible());
      this->GetScene()->SetDirty(true);
    }
  }
}
