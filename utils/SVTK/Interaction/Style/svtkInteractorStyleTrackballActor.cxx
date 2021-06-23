/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTrackballActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleTrackballActor.h"

#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkProp3D.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkInteractorStyleTrackballActor);

//----------------------------------------------------------------------------
svtkInteractorStyleTrackballActor::svtkInteractorStyleTrackballActor()
{
  this->MotionFactor = 10.0;
  this->InteractionProp = nullptr;
  this->InteractionPicker = svtkCellPicker::New();
  this->InteractionPicker->SetTolerance(0.001);
}

//----------------------------------------------------------------------------
svtkInteractorStyleTrackballActor::~svtkInteractorStyleTrackballActor()
{
  this->InteractionPicker->Delete();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
  {
    case SVTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_SPIN:
      this->FindPokedRenderer(x, y);
      this->Spin();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_USCALE:
      this->FindPokedRenderer(x, y);
      this->UniformScale();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnLeftButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetShiftKey())
  {
    this->StartPan();
  }
  else if (this->Interactor->GetControlKey())
  {
    this->StartSpin();
  }
  else
  {
    this->StartRotate();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnLeftButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_PAN:
      this->EndPan();
      break;

    case SVTKIS_SPIN:
      this->EndSpin();
      break;

    case SVTKIS_ROTATE:
      this->EndRotate();
      break;
  }

  if (this->Interactor)
  {
    this->ReleaseFocus();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnMiddleButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetControlKey())
  {
    this->StartDolly();
  }
  else
  {
    this->StartPan();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnMiddleButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_DOLLY:
      this->EndDolly();
      break;

    case SVTKIS_PAN:
      this->EndPan();
      break;
  }

  if (this->Interactor)
  {
    this->ReleaseFocus();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnRightButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  this->FindPickedActor(x, y);
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartUniformScale();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::OnRightButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_USCALE:
      this->EndUniformScale();
      break;
  }

  if (this->Interactor)
  {
    this->ReleaseFocus();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::Rotate()
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

  // First get the origin of the assembly
  double* obj_center = this->InteractionProp->GetCenter();

  // GetLength gets the length of the diagonal of the bounding box
  double boundRadius = this->InteractionProp->GetLength() * 0.5;

  // Get the view up and view right vectors
  double view_up[3], view_look[3], view_right[3];

  cam->OrthogonalizeViewUp();
  cam->ComputeViewPlaneNormal();
  cam->GetViewUp(view_up);
  svtkMath::Normalize(view_up);
  cam->GetViewPlaneNormal(view_look);
  svtkMath::Cross(view_up, view_look, view_right);
  svtkMath::Normalize(view_right);

  // Get the furtherest point from object position+origin
  double outsidept[3];

  outsidept[0] = obj_center[0] + view_right[0] * boundRadius;
  outsidept[1] = obj_center[1] + view_right[1] * boundRadius;
  outsidept[2] = obj_center[2] + view_right[2] * boundRadius;

  // Convert them to display coord
  double disp_obj_center[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], disp_obj_center);

  this->ComputeWorldToDisplay(outsidept[0], outsidept[1], outsidept[2], outsidept);

  double radius = sqrt(svtkMath::Distance2BetweenPoints(disp_obj_center, outsidept));
  double nxf = (rwi->GetEventPosition()[0] - disp_obj_center[0]) / radius;

  double nyf = (rwi->GetEventPosition()[1] - disp_obj_center[1]) / radius;

  double oxf = (rwi->GetLastEventPosition()[0] - disp_obj_center[0]) / radius;

  double oyf = (rwi->GetLastEventPosition()[1] - disp_obj_center[1]) / radius;

  if (((nxf * nxf + nyf * nyf) <= 1.0) && ((oxf * oxf + oyf * oyf) <= 1.0))
  {
    double newXAngle = svtkMath::DegreesFromRadians(asin(nxf));
    double newYAngle = svtkMath::DegreesFromRadians(asin(nyf));
    double oldXAngle = svtkMath::DegreesFromRadians(asin(oxf));
    double oldYAngle = svtkMath::DegreesFromRadians(asin(oyf));

    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.0;

    double** rotate = new double*[2];

    rotate[0] = new double[4];
    rotate[1] = new double[4];

    rotate[0][0] = newXAngle - oldXAngle;
    rotate[0][1] = view_up[0];
    rotate[0][2] = view_up[1];
    rotate[0][3] = view_up[2];

    rotate[1][0] = oldYAngle - newYAngle;
    rotate[1][1] = view_right[0];
    rotate[1][2] = view_right[1];
    rotate[1][3] = view_right[2];

    this->Prop3DTransform(this->InteractionProp, obj_center, 2, rotate, scale);

    delete[] rotate[0];
    delete[] rotate[1];
    delete[] rotate;

    if (this->AutoAdjustCameraClippingRange)
    {
      this->CurrentRenderer->ResetCameraClippingRange();
    }

    rwi->Render();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::Spin()
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

  // Get the axis to rotate around = vector from eye to origin

  double* obj_center = this->InteractionProp->GetCenter();

  double motion_vector[3];
  double view_point[3];

  if (cam->GetParallelProjection())
  {
    // If parallel projection, want to get the view plane normal...
    cam->ComputeViewPlaneNormal();
    cam->GetViewPlaneNormal(motion_vector);
  }
  else
  {
    // Perspective projection, get vector from eye to center of actor
    cam->GetPosition(view_point);
    motion_vector[0] = view_point[0] - obj_center[0];
    motion_vector[1] = view_point[1] - obj_center[1];
    motion_vector[2] = view_point[2] - obj_center[2];
    svtkMath::Normalize(motion_vector);
  }

  double disp_obj_center[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], disp_obj_center);

  double newAngle =
    svtkMath::DegreesFromRadians(atan2(rwi->GetEventPosition()[1] - disp_obj_center[1],
      rwi->GetEventPosition()[0] - disp_obj_center[0]));

  double oldAngle =
    svtkMath::DegreesFromRadians(atan2(rwi->GetLastEventPosition()[1] - disp_obj_center[1],
      rwi->GetLastEventPosition()[0] - disp_obj_center[0]));

  double scale[3];
  scale[0] = scale[1] = scale[2] = 1.0;

  double** rotate = new double*[1];
  rotate[0] = new double[4];

  rotate[0][0] = newAngle - oldAngle;
  rotate[0][1] = motion_vector[0];
  rotate[0][2] = motion_vector[1];
  rotate[0][3] = motion_vector[2];

  this->Prop3DTransform(this->InteractionProp, obj_center, 1, rotate, scale);

  delete[] rotate[0];
  delete[] rotate;

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::Pan()
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  // Use initial center as the origin from which to pan

  double* obj_center = this->InteractionProp->GetCenter();

  double disp_obj_center[3], new_pick_point[4];
  double old_pick_point[4], motion_vector[3];

  this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], disp_obj_center);

  this->ComputeDisplayToWorld(
    rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], disp_obj_center[2], new_pick_point);

  this->ComputeDisplayToWorld(rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1],
    disp_obj_center[2], old_pick_point);

  motion_vector[0] = new_pick_point[0] - old_pick_point[0];
  motion_vector[1] = new_pick_point[1] - old_pick_point[1];
  motion_vector[2] = new_pick_point[2] - old_pick_point[2];

  if (this->InteractionProp->GetUserMatrix() != nullptr)
  {
    svtkTransform* t = svtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
  }
  else
  {
    this->InteractionProp->AddPosition(motion_vector[0], motion_vector[1], motion_vector[2]);
  }

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::Dolly()
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

  double view_point[3], view_focus[3];
  double motion_vector[3];

  cam->GetPosition(view_point);
  cam->GetFocalPoint(view_focus);

  double* center = this->CurrentRenderer->GetCenter();

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double yf = dy / center[1] * this->MotionFactor;
  double dollyFactor = pow(1.1, yf);

  dollyFactor -= 1.0;
  motion_vector[0] = (view_point[0] - view_focus[0]) * dollyFactor;
  motion_vector[1] = (view_point[1] - view_focus[1]) * dollyFactor;
  motion_vector[2] = (view_point[2] - view_focus[2]) * dollyFactor;

  if (this->InteractionProp->GetUserMatrix() != nullptr)
  {
    svtkTransform* t = svtkTransform::New();
    t->PostMultiply();
    t->SetMatrix(this->InteractionProp->GetUserMatrix());
    t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
    this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
    t->Delete();
  }
  else
  {
    this->InteractionProp->AddPosition(motion_vector);
  }

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::UniformScale()
{
  if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  double* obj_center = this->InteractionProp->GetCenter();
  double* center = this->CurrentRenderer->GetCenter();

  double yf = dy / center[1] * this->MotionFactor;
  double scaleFactor = pow(1.1, yf);

  double** rotate = nullptr;

  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;

  this->Prop3DTransform(this->InteractionProp, obj_center, 0, rotate, scale);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::FindPickedActor(int x, int y)
{
  this->InteractionPicker->Pick(x, y, 0.0, this->CurrentRenderer);
  svtkProp* prop = this->InteractionPicker->GetViewProp();
  if (prop != nullptr)
  {
    this->InteractionProp = svtkProp3D::SafeDownCast(prop);
  }
  else
  {
    this->InteractionProp = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballActor::Prop3DTransform(
  svtkProp3D* prop3D, double* boxCenter, int numRotation, double** rotate, double* scale)
{
  svtkMatrix4x4* oldMatrix = svtkMatrix4x4::New();
  prop3D->GetMatrix(oldMatrix);

  double orig[3];
  prop3D->GetOrigin(orig);

  svtkTransform* newTransform = svtkTransform::New();
  newTransform->PostMultiply();
  if (prop3D->GetUserMatrix() != nullptr)
  {
    newTransform->SetMatrix(prop3D->GetUserMatrix());
  }
  else
  {
    newTransform->SetMatrix(oldMatrix);
  }

  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));

  for (int i = 0; i < numRotation; i++)
  {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1], rotate[i][2], rotate[i][3]);
  }

  if ((scale[0] * scale[1] * scale[2]) != 0.0)
  {
    newTransform->Scale(scale[0], scale[1], scale[2]);
  }

  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);

  // now try to get the composite of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);

  if (prop3D->GetUserMatrix() != nullptr)
  {
    newTransform->GetMatrix(prop3D->GetUserMatrix());
  }
  else
  {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
  }
  oldMatrix->Delete();
  newTransform->Delete();
}
