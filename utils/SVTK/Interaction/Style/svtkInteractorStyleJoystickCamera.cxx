/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleJoystickCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleJoystickCamera.h"

#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkInteractorStyleJoystickCamera);

//----------------------------------------------------------------------------
svtkInteractorStyleJoystickCamera::svtkInteractorStyleJoystickCamera()
{
  // Use timers to handle continuous interaction

  this->UseTimers = 1;
}

//----------------------------------------------------------------------------
svtkInteractorStyleJoystickCamera::~svtkInteractorStyleJoystickCamera() = default;

//----------------------------------------------------------------------------
// Mouse events
//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
  {
    case SVTKIS_ROTATE:
    case SVTKIS_PAN:
    case SVTKIS_DOLLY:
    case SVTKIS_SPIN:
      this->FindPokedRenderer(x, y);
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::OnLeftButtonDown()
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
void svtkInteractorStyleJoystickCamera::OnLeftButtonUp()
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
void svtkInteractorStyleJoystickCamera::OnMiddleButtonDown()
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
void svtkInteractorStyleJoystickCamera::OnMiddleButtonUp()
{
  switch (this->State)
  {
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
void svtkInteractorStyleJoystickCamera::OnRightButtonDown()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::OnRightButtonUp()
{
  switch (this->State)
  {
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
void svtkInteractorStyleJoystickCamera::OnMouseWheelForward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = 10.0 * 0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::OnMouseWheelBackward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = 10.0 * -0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::Rotate()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  double* center = this->CurrentRenderer->GetCenter();

  double dx = rwi->GetEventPosition()[0] - center[0];
  double dy = rwi->GetEventPosition()[1] - center[1];

  double* vp = this->CurrentRenderer->GetViewport();
  const int* size = rwi->GetSize();

  double delta_elevation = -20.0 / ((vp[3] - vp[1]) * size[1]);
  double delta_azimuth = -20.0 / ((vp[2] - vp[0]) * size[0]);

  double rxf = dx * delta_azimuth;
  double ryf = dy * delta_elevation;

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
void svtkInteractorStyleJoystickCamera::Spin()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  double* center = this->CurrentRenderer->GetCenter();

  // Spin is based on y value

  double yf = (rwi->GetEventPosition()[1] - center[1]) / center[1];

  if (yf > 1.)
  {
    yf = 1.;
  }
  else if (yf < -1.)
  {
    yf = -1.;
  }

  double newAngle = svtkMath::DegreesFromRadians(asin(yf));

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll(newAngle);
  camera->OrthogonalizeViewUp();

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::Pan()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;

  double ViewFocus[4];
  double NewPickPoint[4];

  // Calculate the focal depth since we'll be using it a lot

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetFocalPoint(ViewFocus);
  this->ComputeWorldToDisplay(ViewFocus[0], ViewFocus[1], ViewFocus[2], ViewFocus);
  double focalDepth = ViewFocus[2];

  this->ComputeDisplayToWorld(
    rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], focalDepth, NewPickPoint);

  // Get the current focal point and position

  camera->GetFocalPoint(ViewFocus);
  double* ViewPoint = camera->GetPosition();

  // Compute a translation vector, moving everything 1/10
  // the distance to the cursor. (Arbitrary scale factor)

  double MotionVector[3];
  MotionVector[0] = 0.1 * (ViewFocus[0] - NewPickPoint[0]);
  MotionVector[1] = 0.1 * (ViewFocus[1] - NewPickPoint[1]);
  MotionVector[2] = 0.1 * (ViewFocus[2] - NewPickPoint[2]);

  camera->SetFocalPoint(
    MotionVector[0] + ViewFocus[0], MotionVector[1] + ViewFocus[1], MotionVector[2] + ViewFocus[2]);

  camera->SetPosition(
    MotionVector[0] + ViewPoint[0], MotionVector[1] + ViewPoint[1], MotionVector[2] + ViewPoint[2]);

  if (rwi->GetLightFollowCamera())
  {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
  }

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::Dolly()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  double* center = this->CurrentRenderer->GetCenter();
  double dy = rwi->GetEventPosition()[1] - center[1];
  double dyf = 0.5 * dy / center[1];
  this->Dolly(pow(1.1, dyf));
}

//----------------------------------------------------------------------------
void svtkInteractorStyleJoystickCamera::Dolly(double factor)
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
void svtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
