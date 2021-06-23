/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleTrackballCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleTrackballCamera.h"

#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkMatrix3x3.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkInteractorStyleTrackballCamera);

//----------------------------------------------------------------------------
svtkInteractorStyleTrackballCamera::svtkInteractorStyleTrackballCamera()
{
  this->MotionFactor = 10.0;
}

//----------------------------------------------------------------------------
svtkInteractorStyleTrackballCamera::~svtkInteractorStyleTrackballCamera() = default;

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
  {
    case SVTKIS_ENV_ROTATE:
      this->FindPokedRenderer(x, y);
      this->EnvironmentRotate();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

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
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnLeftButtonDown()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetShiftKey())
  {
    if (this->Interactor->GetControlKey())
    {
      this->StartDolly();
    }
    else
    {
      this->StartPan();
    }
  }
  else
  {
    if (this->Interactor->GetControlKey())
    {
      this->StartSpin();
    }
    else
    {
      this->StartRotate();
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnLeftButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_DOLLY:
      this->EndDolly();
      break;

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
void svtkInteractorStyleTrackballCamera::OnMiddleButtonDown()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartPan();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnMiddleButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_PAN:
      this->EndPan();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnRightButtonDown()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);

  if (this->Interactor->GetShiftKey())
  {
    this->StartEnvRotate();
  }
  else
  {
    this->StartDolly();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnRightButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_ENV_ROTATE:
      this->EndEnvRotate();
      break;

    case SVTKIS_DOLLY:
      this->EndDolly();
      break;
  }

  if (this->Interactor)
  {
    this->ReleaseFocus();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnMouseWheelForward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->MotionFactor * 0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::OnMouseWheelBackward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->MotionFactor * -0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::Rotate()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  int dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  const int* size = this->CurrentRenderer->GetRenderWindow()->GetSize();

  double delta_elevation = -20.0 / size[1];
  double delta_azimuth = -20.0 / size[0];

  double rxf = dx * delta_azimuth * this->MotionFactor;
  double ryf = dy * delta_elevation * this->MotionFactor;

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth(rxf);
  camera->Elevation(ryf);
  camera->OrthogonalizeViewUp();

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }

  if (rwi->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::Spin()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  double* center = this->CurrentRenderer->GetCenter();

  double newAngle = svtkMath::DegreesFromRadians(
    atan2(rwi->GetEventPosition()[1] - center[1], rwi->GetEventPosition()[0] - center[0]));

  double oldAngle = svtkMath::DegreesFromRadians(
    atan2(rwi->GetLastEventPosition()[1] - center[1], rwi->GetLastEventPosition()[0] - center[0]));

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll(newAngle - oldAngle);
  camera->OrthogonalizeViewUp();

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::Pan()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];

  // Calculate the focal depth since we'll be using it a lot

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld(
    rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], focalDepth, newPickPoint);

  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop

  this->ComputeDisplayToWorld(
    rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1], focalDepth, oldPickPoint);

  // Camera motion is reversed

  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];

  camera->GetFocalPoint(viewFocus);
  camera->GetPosition(viewPoint);
  camera->SetFocalPoint(
    motionVector[0] + viewFocus[0], motionVector[1] + viewFocus[1], motionVector[2] + viewFocus[2]);

  camera->SetPosition(
    motionVector[0] + viewPoint[0], motionVector[1] + viewPoint[1], motionVector[2] + viewPoint[2]);

  if (rwi->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::Dolly()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  double* center = this->CurrentRenderer->GetCenter();
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double dyf = this->MotionFactor * dy / center[1];
  this->Dolly(pow(1.1, dyf));
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::Dolly(double factor)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (camera->GetParallelProjection())
  {
    camera->SetParallelScale(camera->GetParallelScale() / factor);
  }
  else
  {
    camera->Dolly(factor);
    if (this->AutoAdjustCameraClippingRange)
    {
      this->CurrentRenderer->ResetCameraClippingRange();
    }
  }

  if (this->Interactor->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::EnvironmentRotate()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  int dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  int sizeX = this->CurrentRenderer->GetRenderWindow()->GetSize()[0];

  svtkNew<svtkMatrix3x3> mat;

  double* up = this->CurrentRenderer->GetEnvironmentUp();
  double* right = this->CurrentRenderer->GetEnvironmentRight();

  double front[3];
  svtkMath::Cross(right, up, front);
  for (int i = 0; i < 3; i++)
  {
    mat->SetElement(i, 0, right[i]);
    mat->SetElement(i, 1, up[i]);
    mat->SetElement(i, 2, front[i]);
  }

  double angle = (dx / static_cast<double>(sizeX)) * this->MotionFactor;

  double c = std::cos(angle);
  double s = std::sin(angle);
  double t = 1.0 - c;

  svtkNew<svtkMatrix3x3> rot;

  rot->SetElement(0, 0, t * up[0] * up[0] + c);
  rot->SetElement(0, 1, t * up[0] * up[1] - up[2] * s);
  rot->SetElement(0, 2, t * up[0] * up[2] + up[1] * s);

  rot->SetElement(1, 0, t * up[0] * up[1] + up[2] * s);
  rot->SetElement(1, 1, t * up[1] * up[1] + c);
  rot->SetElement(1, 2, t * up[1] * up[2] - up[0] * s);

  rot->SetElement(2, 0, t * up[0] * up[2] - up[1] * s);
  rot->SetElement(2, 1, t * up[1] * up[2] + up[0] * s);
  rot->SetElement(2, 2, t * up[2] * up[2] + c);

  svtkMatrix3x3::Multiply3x3(rot, mat, mat);

  // update environment orientation
  this->CurrentRenderer->SetEnvironmentUp(
    mat->GetElement(0, 1), mat->GetElement(1, 1), mat->GetElement(2, 1));
  this->CurrentRenderer->SetEnvironmentRight(
    mat->GetElement(0, 0), mat->GetElement(1, 0), mat->GetElement(2, 0));

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleTrackballCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MotionFactor: " << this->MotionFactor << "\n";
}
