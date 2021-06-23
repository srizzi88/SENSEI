/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyleCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTDxInteractorStyleCamera.h"

#include "svtkCamera.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTDxInteractorStyleSettings.h"
#include "svtkTDxMotionEventInfo.h"
#include "svtkTransform.h"
#include <cassert>

svtkStandardNewMacro(svtkTDxInteractorStyleCamera);

// ----------------------------------------------------------------------------
svtkTDxInteractorStyleCamera::svtkTDxInteractorStyleCamera()
{
  this->Transform = svtkTransform::New();
  //  this->DebugOn();
}

// ----------------------------------------------------------------------------
svtkTDxInteractorStyleCamera::~svtkTDxInteractorStyleCamera()
{
  this->Transform->Delete();
}

// ----------------------------------------------------------------------------
void svtkTDxInteractorStyleCamera::OnMotionEvent(svtkTDxMotionEventInfo* motionInfo)
{
  assert("pre: motionInfo_exist" && motionInfo != nullptr);

  svtkDebugMacro(<< "svtkTDxInteractorStyleCamera::OnMotionEvent()");

  if (this->Renderer == nullptr || this->Settings == nullptr)
  {
    svtkDebugMacro(<< "svtkTDxInteractorStyleCamera::OnMotionEvent() no renderer or no settings");
    return;
  }

  svtkCamera* c = this->Renderer->GetActiveCamera();
  svtkRenderWindow* w = this->Renderer->GetRenderWindow();
  svtkRenderWindowInteractor* i = w->GetInteractor();

  svtkDebugMacro(<< "x=" << motionInfo->X << " y=" << motionInfo->Y << " z=" << motionInfo->Z
                << " angle=" << motionInfo->Angle << " rx=" << motionInfo->AxisX
                << " ry=" << motionInfo->AxisY << " rz=" << motionInfo->AxisZ);

  svtkTransform* eyeToWorld = c->GetViewTransformObject();
  double axisEye[3];
  double axisWorld[3];

  // As we want to rotate the camera, the incoming data are expressed in
  // eye coordinates.
  if (this->Settings->GetUseRotationX())
  {
    axisEye[0] = motionInfo->AxisX;
  }
  else
  {
    axisEye[0] = 0.0;
  }
  if (this->Settings->GetUseRotationY())
  {
    axisEye[1] = motionInfo->AxisY;
  }
  else
  {
    axisEye[1] = 0.0;
  }
  if (this->Settings->GetUseRotationZ())
  {
    axisEye[2] = motionInfo->AxisZ;
  }
  else
  {
    axisEye[2] = 0.0;
  }

  // Get the rotation axis in world coordinates.
  this->Transform->Identity();
  this->Transform->Concatenate(eyeToWorld);
  this->Transform->Inverse();
  this->Transform->TransformVector(axisEye, axisWorld);

  // Get the translation vector in world coordinates. Used at the end.
  double translationEye[3];
  double translationWorld[3];
  translationEye[0] = motionInfo->X * this->Settings->GetTranslationXSensitivity();
  translationEye[1] = motionInfo->Y * this->Settings->GetTranslationYSensitivity();
  translationEye[2] = motionInfo->Z * this->Settings->GetTranslationZSensitivity();
  this->Transform->TransformVector(translationEye, translationWorld);

  this->Transform->Identity();

  // default multiplication is "pre" which means applied to the "right" of
  // the current matrix, which follows the OpenGL multiplication convention.

  // 2. translate (affect position and focalPoint)
  this->Transform->Translate(translationWorld);

  // 1. build the displacement (aka affine rotation) with the axis
  // passing through the focal point.

  double* p = c->GetFocalPoint();
  this->Transform->Translate(p[0], p[1], p[2]);
  this->Transform->RotateWXYZ(motionInfo->Angle * this->Settings->GetAngleSensitivity(),
    axisWorld[0], axisWorld[1], axisWorld[2]);
  this->Transform->Translate(-p[0], -p[1], -p[2]);

  // Apply the transform to the camera position
  double* pos = c->GetPosition();
  double newPosition[3];
  this->Transform->TransformPoint(pos, newPosition);

  // Apply the vector part of the transform to the camera view up vector
  double* up = c->GetViewUp();
  double newUp[3];
  this->Transform->TransformVector(up, newUp);

  // Apply the transform to the camera position
  double newFocalPoint[3];
  this->Transform->TransformPoint(p, newFocalPoint);

  // Set the new view up vector and position of the camera.
  c->SetViewUp(newUp);
  c->SetPosition(newPosition);
  c->SetFocalPoint(newFocalPoint);

  this->Renderer->ResetCameraClippingRange();

  // Display the result.
  i->Render();
}

//----------------------------------------------------------------------------
void svtkTDxInteractorStyleCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
