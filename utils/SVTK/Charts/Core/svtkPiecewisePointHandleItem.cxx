/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewisePointHandleItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPiecewisePointHandleItem.h"

#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkControlPointsItem.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"

#include <algorithm>

enum enumPointHandleType
{
  enSharpNess = 0,
  enMidPoint
};

class PointHandle
{
public:
  void Init(float x, float y, svtkIdType idx, enumPointHandleType type, float val, float distance,
    svtkVector2f& sceneOrigin)
  {
    this->Position[0] = x;
    this->Position[1] = y;
    this->PointIndex = idx;
    this->enType = type;
    this->fValue = val;
    this->fDistance = distance;
    this->ScenePos[0] = sceneOrigin[0] + x;
    this->ScenePos[1] = sceneOrigin[1] + y;
  }
  void DrawCircle(svtkContext2D* painter, float radius)
  {
    painter->DrawArc(this->Position[0], this->Position[1], radius, 0.f, 360.f);
  }
  float Position[2];
  float ScenePos[2];
  svtkIdType PointIndex;
  enumPointHandleType enType;
  float fValue;
  float fDistance;
};

class svtkPiecewisePointHandleItem::InternalPiecewisePointHandleInfo
{
public:
  PointHandle PointHandles[4];
};
//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPiecewisePointHandleItem);

//-----------------------------------------------------------------------------
svtkPiecewisePointHandleItem::svtkPiecewisePointHandleItem()
{
  this->MouseOverHandleIndex = -1;
  this->PiecewiseFunction = nullptr;
  this->Callback = svtkCallbackCommand::New();
  this->Callback->SetClientData(this);
  this->Callback->SetCallback(svtkPiecewisePointHandleItem::CallRedraw);
  this->HandleRadius = 3.f;
  this->CurrentPointIndex = -1;
  this->Internal = new InternalPiecewisePointHandleInfo();
}

//-----------------------------------------------------------------------------
svtkPiecewisePointHandleItem::~svtkPiecewisePointHandleItem()
{
  this->SetPiecewiseFunction(nullptr);
  if (this->Callback)
  {
    this->Callback->Delete();
    this->Callback = nullptr;
  }
  delete this->Internal;
}
// ----------------------------------------------------------------------------
void svtkPiecewisePointHandleItem::SetParent(svtkAbstractContextItem* parent)
{
  if (this->Parent == parent)
  {
    return;
  }
  else if (this->Parent)
  {
    if (this->PiecewiseFunction)
    {
      this->Parent->RemoveObserver(this->Callback);
    }
  }
  this->Superclass::SetParent(parent);
  if (parent)
  {
    this->Parent->AddObserver(svtkControlPointsItem::CurrentPointChangedEvent, this->Callback);
  }
}

//-----------------------------------------------------------------------------
bool svtkPiecewisePointHandleItem::Paint(svtkContext2D* painter)
{
  svtkControlPointsItem* parentControl = svtkControlPointsItem::SafeDownCast(this->GetParent());
  if (!parentControl || parentControl->GetCurrentPoint() < 0 || !this->GetPiecewiseFunction())
  {
    this->CurrentPointIndex = -1;
    return true;
  }
  svtkIdType currentIdx = parentControl->GetCurrentPoint();
  this->CurrentPointIndex = currentIdx;
  double point[4];
  parentControl->GetControlPoint(parentControl->GetCurrentPoint(), point);

  // transform from data space to rendering space.
  svtkVector2f dataPoint(static_cast<float>(point[0]), static_cast<float>(point[1]));
  svtkVector2f screenPoint;
  parentControl->TransformDataToScreen(dataPoint, screenPoint);

  unsigned char brushOpacity = painter->GetBrush()->GetOpacity();
  unsigned char penColor[3];
  painter->GetPen()->GetColor(penColor);
  unsigned char penOpacity = painter->GetPen()->GetOpacity();

  svtkVector2f pointInScene;
  svtkTransform2D* sceneTransform = painter->GetTransform();
  sceneTransform->TransformPoints(screenPoint.GetData(), pointInScene.GetData(), 1);

  svtkNew<svtkTransform2D> translation;
  translation->Translate(pointInScene[0], pointInScene[1]);

  painter->PushMatrix();
  painter->SetTransform(translation);
  painter->GetPen()->SetColor(0, 200, 0);

  float radius = this->HandleRadius;
  svtkIdType preIdx = currentIdx - 1;
  svtkIdType nxtIdx = currentIdx + 1;
  double preMid = 0.0, preSharp = 0.0, curMid = point[2], curSharp = point[3];
  double prePoint[4], nxtPoint[4];
  if (preIdx >= 0)
  {
    this->PiecewiseFunction->GetNodeValue(preIdx, prePoint);
    preMid = prePoint[2];
    preSharp = prePoint[3];
  }
  if (nxtIdx < parentControl->GetNumberOfPoints())
  {
    this->PiecewiseFunction->GetNodeValue(nxtIdx, nxtPoint);
    preMid = prePoint[2];
    preSharp = prePoint[3];
  }

  // The following calculations are to find out the correct
  // handle positions to draw. The handle positions are relative
  // to the point position and they are in scene units.
  // The distance from the current point to previous and next points
  // are also cached, so that it will be convenient to convert
  // mouse movement to corresponding midpoint/sharpness changes.

  float ptRadius = parentControl->GetScreenPointRadius();
  float fDistance = this->HandleRadius + ptRadius;

  svtkVector2f blPosData(prePoint[0], prePoint[1]);
  svtkVector2f trPosData(nxtPoint[0], nxtPoint[1]);

  svtkVector2f blPosScreen;
  svtkVector2f trPosScreen;

  parentControl->TransformDataToScreen(blPosData, blPosScreen);
  parentControl->TransformDataToScreen(trPosData, trPosScreen);

  sceneTransform->TransformPoints(blPosScreen.GetData(), blPosScreen.GetData(), 1);
  sceneTransform->TransformPoints(trPosScreen.GetData(), trPosScreen.GetData(), 1);

  double blxdistance = fabs(pointInScene[0] - blPosScreen[0]) - fDistance * 2.0;
  double blydistance = fabs(pointInScene[1] - blPosScreen[1]) - fDistance * 2.0;
  double trxdistance = fabs(pointInScene[0] - trPosScreen[0]) - fDistance * 2.0;
  double trydistance = fabs(pointInScene[1] - trPosScreen[1]) - fDistance * 2.0;

  blxdistance = std::max(0.0, blxdistance);
  blydistance = std::max(0.0, blydistance);
  trxdistance = std::max(0.0, trxdistance);
  trydistance = std::max(0.0, trydistance);

  this->Internal->PointHandles[0].Init(0.f, fDistance + trydistance * curSharp, currentIdx,
    enSharpNess, curSharp, trydistance, pointInScene);
  this->Internal->PointHandles[1].Init(fDistance + trxdistance * curMid, 0.f, currentIdx,
    enMidPoint, curMid, trxdistance, pointInScene);
  this->Internal->PointHandles[2].Init(0.f, -(fDistance + blydistance * preSharp), preIdx,
    enSharpNess, preSharp, blydistance, pointInScene);
  this->Internal->PointHandles[3].Init(-(fDistance + blxdistance * (1 - preMid)), 0.f, preIdx,
    enMidPoint, preMid, blxdistance, pointInScene);

  if (ptRadius + trydistance * curSharp != ptRadius)
  {
    painter->DrawLine(0, ptRadius + trydistance * curSharp, 0, ptRadius);
  }
  if (ptRadius != ptRadius + trxdistance * curMid)
  {
    painter->DrawLine(ptRadius, 0, ptRadius + trxdistance * curMid, 0);
  }
  if (ptRadius + blydistance * preSharp != ptRadius)
  {
    painter->DrawLine(-(ptRadius + blxdistance * (1 - preMid)), 0, -ptRadius, 0);
  }
  if (ptRadius + blxdistance * (1 - preMid) != ptRadius)
  {
    painter->DrawLine(-(ptRadius + blxdistance * (1 - preMid)), 0, -ptRadius, 0);
  }

  for (int i = 0; i < 4; i++)
  {
    if (i == this->MouseOverHandleIndex)
    {
      painter->GetBrush()->SetColor(255, 0, 255);
    }
    else
    {
      painter->GetBrush()->SetColor(0, 200, 0);
    }
    this->Internal->PointHandles[i].DrawCircle(painter, radius);
  }

  painter->PopMatrix();
  painter->GetPen()->SetColor(penColor);
  painter->GetPen()->SetOpacity(penOpacity);
  painter->GetBrush()->SetOpacity(brushOpacity);

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkPiecewisePointHandleItem::Hit(const svtkContextMouseEvent& mouse)
{
  float pos[2] = { mouse.GetScenePos().GetX(), mouse.GetScenePos().GetY() };
  if (this->IsOverHandle(pos) >= 0)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
int svtkPiecewisePointHandleItem::IsOverHandle(float* scenePos)
{
  svtkControlPointsItem* parentControl = svtkControlPointsItem::SafeDownCast(this->GetParent());
  if (!parentControl || parentControl->GetCurrentPoint() < 0 || !this->GetPiecewiseFunction() ||
    !this->Scene->GetLastPainter())
  {
    return -1;
  }

  // we have four screen handles to check
  for (int i = 0; i < 4; ++i)
  {
    double sceneHandlePoint[2] = { this->Internal->PointHandles[i].ScenePos[0],
      this->Internal->PointHandles[i].ScenePos[1] };
    double distance2 = (sceneHandlePoint[0] - scenePos[0]) * (sceneHandlePoint[0] - scenePos[0]) +
      (sceneHandlePoint[1] - scenePos[1]) * (sceneHandlePoint[1] - scenePos[1]);
    double tolerance = 1.5;
    double radius2 = this->HandleRadius * this->HandleRadius * tolerance * tolerance;
    if (distance2 <= radius2)
    {
      return i;
    }
  }

  return -1;
}

//-----------------------------------------------------------------------------
bool svtkPiecewisePointHandleItem::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    if (this->MouseOverHandleIndex >= 0)
    {
      PointHandle* activeHandle = &this->Internal->PointHandles[this->MouseOverHandleIndex];
      float deltaX = mouse.GetScenePos().GetX() - activeHandle->ScenePos[0];
      float deltaY = mouse.GetScenePos().GetY() - activeHandle->ScenePos[1];

      svtkControlPointsItem* parentControl = svtkControlPointsItem::SafeDownCast(this->GetParent());
      if (activeHandle->fDistance <= 0 || !parentControl || parentControl->GetCurrentPoint() < 0 ||
        !this->GetPiecewiseFunction())
      {
        return false;
      }
      svtkIdType curIdx = activeHandle->PointIndex;
      double point[4];
      this->PiecewiseFunction->GetNodeValue(curIdx, point);
      if (activeHandle->enType == enMidPoint)
      {
        double fMid = deltaX / activeHandle->fDistance + activeHandle->fValue;
        fMid = std::max(fMid, 0.0);
        fMid = std::min(fMid, 1.0);
        point[2] = fMid;
      }
      else
      {
        if (this->MouseOverHandleIndex == 2)
        {
          deltaY = -deltaY;
        }
        double fSharp = deltaY / activeHandle->fDistance + activeHandle->fValue;
        fSharp = std::max(fSharp, 0.0);
        fSharp = std::min(fSharp, 1.0);
        point[3] = fSharp;
      }
      this->GetPiecewiseFunction()->SetNodeValue(curIdx, point);
      return true;
    }
  }
  else if (mouse.GetButton() == svtkContextMouseEvent::NO_BUTTON)
  {
    float mspos[2] = { mouse.GetScenePos().GetX(), mouse.GetScenePos().GetY() };
    int handleIdx = this->IsOverHandle(mspos);
    if (this->MouseOverHandleIndex != handleIdx)
    {
      this->MouseOverHandleIndex = handleIdx;
      this->GetScene()->SetDirty(true);
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool svtkPiecewisePointHandleItem::MouseButtonPressEvent(const svtkContextMouseEvent&)
{
  if (this->MouseOverHandleIndex >= 0)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkPiecewisePointHandleItem::MouseButtonReleaseEvent(const svtkContextMouseEvent&)
{
  if (this->MouseOverHandleIndex >= 0)
  {
    this->MouseOverHandleIndex = -1;
    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
svtkWeakPointer<svtkPiecewiseFunction> svtkPiecewisePointHandleItem::GetPiecewiseFunction()
{
  return this->PiecewiseFunction;
}

//-----------------------------------------------------------------------------
void svtkPiecewisePointHandleItem::SetPiecewiseFunction(svtkPiecewiseFunction* function)
{
  if (function == this->PiecewiseFunction)
  {
    return;
  }
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
  }
  this->PiecewiseFunction = function;
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->AddObserver(svtkCommand::ModifiedEvent, this->Callback);
    this->PiecewiseFunction->AddObserver(svtkCommand::EndEvent, this->Callback);
  }
  this->Redraw();
}
//-----------------------------------------------------------------------------
void svtkPiecewisePointHandleItem::Redraw()
{
  if (this->Scene)
  {
    this->Scene->SetDirty(true);
  }
}
//-----------------------------------------------------------------------------
void svtkPiecewisePointHandleItem::CallRedraw(
  svtkObject* svtkNotUsed(sender), unsigned long event, void* receiver, void* svtkNotUsed(params))
{
  svtkPiecewisePointHandleItem* item = reinterpret_cast<svtkPiecewisePointHandleItem*>(receiver);
  switch (event)
  {
    case svtkCommand::ModifiedEvent:
    case svtkCommand::EndEvent:
    case svtkControlPointsItem::CurrentPointChangedEvent:
      item->Redraw();
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
void svtkPiecewisePointHandleItem::PrintSelf(ostream& os, svtkIndent indent)
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
  os << indent << "MouseOverHandleIndex: " << this->MouseOverHandleIndex << endl;
  os << indent << "CurrentPointIndex: " << this->CurrentPointIndex << endl;
}
