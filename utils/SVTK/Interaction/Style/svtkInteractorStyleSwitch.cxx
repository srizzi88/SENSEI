/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleSwitch.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleSwitch.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkInteractorStyleJoystickActor.h"
#include "svtkInteractorStyleJoystickCamera.h"
#include "svtkInteractorStyleMultiTouchCamera.h"
#include "svtkInteractorStyleTrackballActor.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"

svtkStandardNewMacro(svtkInteractorStyleSwitch);

//----------------------------------------------------------------------------
svtkInteractorStyleSwitch::svtkInteractorStyleSwitch()
{
  this->JoystickActor = svtkInteractorStyleJoystickActor::New();
  this->JoystickCamera = svtkInteractorStyleJoystickCamera::New();
  this->TrackballActor = svtkInteractorStyleTrackballActor::New();
  this->TrackballCamera = svtkInteractorStyleTrackballCamera::New();
  this->MultiTouchCamera = svtkInteractorStyleMultiTouchCamera::New();
  this->JoystickOrTrackball = SVTKIS_JOYSTICK;
  this->CameraOrActor = SVTKIS_CAMERA;
  this->MultiTouch = false;
  this->CurrentStyle = nullptr;
}

//----------------------------------------------------------------------------
svtkInteractorStyleSwitch::~svtkInteractorStyleSwitch()
{
  this->JoystickActor->Delete();
  this->JoystickActor = nullptr;

  this->JoystickCamera->Delete();
  this->JoystickCamera = nullptr;

  this->TrackballActor->Delete();
  this->TrackballActor = nullptr;

  this->TrackballCamera->Delete();
  this->TrackballCamera = nullptr;

  this->MultiTouchCamera->Delete();
  this->MultiTouchCamera = nullptr;
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetAutoAdjustCameraClippingRange(svtkTypeBool value)
{
  if (value == this->AutoAdjustCameraClippingRange)
  {
    return;
  }

  if (value < 0 || value > 1)
  {
    svtkErrorMacro("Value must be between 0 and 1 for"
      << " SetAutoAdjustCameraClippingRange");
    return;
  }

  this->AutoAdjustCameraClippingRange = value;
  this->JoystickActor->SetAutoAdjustCameraClippingRange(value);
  this->JoystickCamera->SetAutoAdjustCameraClippingRange(value);
  this->TrackballActor->SetAutoAdjustCameraClippingRange(value);
  this->TrackballCamera->SetAutoAdjustCameraClippingRange(value);
  this->MultiTouchCamera->SetAutoAdjustCameraClippingRange(value);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentStyleToJoystickActor()
{
  this->JoystickOrTrackball = SVTKIS_JOYSTICK;
  this->CameraOrActor = SVTKIS_ACTOR;
  this->MultiTouch = false;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentStyleToJoystickCamera()
{
  this->JoystickOrTrackball = SVTKIS_JOYSTICK;
  this->CameraOrActor = SVTKIS_CAMERA;
  this->MultiTouch = false;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentStyleToTrackballActor()
{
  this->JoystickOrTrackball = SVTKIS_TRACKBALL;
  this->CameraOrActor = SVTKIS_ACTOR;
  this->MultiTouch = false;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentStyleToTrackballCamera()
{
  this->JoystickOrTrackball = SVTKIS_TRACKBALL;
  this->CameraOrActor = SVTKIS_CAMERA;
  this->MultiTouch = false;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentStyleToMultiTouchCamera()
{
  this->MultiTouch = true;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::OnChar()
{
  switch (this->Interactor->GetKeyCode())
  {
    case 'j':
    case 'J':
      this->JoystickOrTrackball = SVTKIS_JOYSTICK;
      this->MultiTouch = false;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 't':
    case 'T':
      this->JoystickOrTrackball = SVTKIS_TRACKBALL;
      this->MultiTouch = false;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 'c':
    case 'C':
      this->CameraOrActor = SVTKIS_CAMERA;
      this->MultiTouch = false;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 'a':
    case 'A':
      this->CameraOrActor = SVTKIS_ACTOR;
      this->MultiTouch = false;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 'm':
    case 'M':
      this->MultiTouch = true;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
  }
  // Set the CurrentStyle pointer to the picked style
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
// this will do nothing if the CurrentStyle matches
// JoystickOrTrackball and CameraOrActor
// It should! If the this->Interactor was changed (using SetInteractor()),
// and the currentstyle should not change.
void svtkInteractorStyleSwitch::SetCurrentStyle()
{
  // if the currentstyle does not match JoystickOrTrackball
  // and CameraOrActor ivars, then call SetInteractor(0)
  // on the Currentstyle to remove all of the observers.
  // Then set the Currentstyle and call SetInteractor with
  // this->Interactor so the callbacks are set for the
  // currentstyle.
  if (this->MultiTouch)
  {
    if (this->CurrentStyle != this->MultiTouchCamera)
    {
      if (this->CurrentStyle)
      {
        this->CurrentStyle->SetInteractor(nullptr);
      }
      this->CurrentStyle = this->MultiTouchCamera;
    }
  }
  else if (this->JoystickOrTrackball == SVTKIS_JOYSTICK && this->CameraOrActor == SVTKIS_CAMERA)
  {
    if (this->CurrentStyle != this->JoystickCamera)
    {
      if (this->CurrentStyle)
      {
        this->CurrentStyle->SetInteractor(nullptr);
      }
      this->CurrentStyle = this->JoystickCamera;
    }
  }
  else if (this->JoystickOrTrackball == SVTKIS_JOYSTICK && this->CameraOrActor == SVTKIS_ACTOR)
  {
    if (this->CurrentStyle != this->JoystickActor)
    {
      if (this->CurrentStyle)
      {
        this->CurrentStyle->SetInteractor(nullptr);
      }
      this->CurrentStyle = this->JoystickActor;
    }
  }
  else if (this->JoystickOrTrackball == SVTKIS_TRACKBALL && this->CameraOrActor == SVTKIS_CAMERA)
  {
    if (this->CurrentStyle != this->TrackballCamera)
    {
      if (this->CurrentStyle)
      {
        this->CurrentStyle->SetInteractor(nullptr);
      }
      this->CurrentStyle = this->TrackballCamera;
    }
  }
  else if (this->JoystickOrTrackball == SVTKIS_TRACKBALL && this->CameraOrActor == SVTKIS_ACTOR)
  {
    if (this->CurrentStyle != this->TrackballActor)
    {
      if (this->CurrentStyle)
      {
        this->CurrentStyle->SetInteractor(nullptr);
      }
      this->CurrentStyle = this->TrackballActor;
    }
  }
  if (this->CurrentStyle)
  {
    this->CurrentStyle->SetInteractor(this->Interactor);
    this->CurrentStyle->SetTDxStyle(this->TDxStyle);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetInteractor(svtkRenderWindowInteractor* iren)
{
  if (iren == this->Interactor)
  {
    return;
  }
  // if we already have an Interactor then stop observing it
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
  }
  this->Interactor = iren;
  // add observers for each of the events handled in ProcessEvents
  if (iren)
  {
    iren->AddObserver(svtkCommand::CharEvent, this->EventCallbackCommand, this->Priority);

    iren->AddObserver(svtkCommand::DeleteEvent, this->EventCallbackCommand, this->Priority);
  }
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CurrentStyle " << this->CurrentStyle << "\n";
  if (this->CurrentStyle)
  {
    svtkIndent next_indent = indent.GetNextIndent();
    os << next_indent << this->CurrentStyle->GetClassName() << "\n";
    this->CurrentStyle->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetDefaultRenderer(svtkRenderer* renderer)
{
  this->svtkInteractorStyle::SetDefaultRenderer(renderer);
  this->JoystickActor->SetDefaultRenderer(renderer);
  this->JoystickCamera->SetDefaultRenderer(renderer);
  this->TrackballActor->SetDefaultRenderer(renderer);
  this->TrackballCamera->SetDefaultRenderer(renderer);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitch::SetCurrentRenderer(svtkRenderer* renderer)
{
  this->svtkInteractorStyle::SetCurrentRenderer(renderer);
  this->JoystickActor->SetCurrentRenderer(renderer);
  this->JoystickCamera->SetCurrentRenderer(renderer);
  this->TrackballActor->SetCurrentRenderer(renderer);
  this->TrackballCamera->SetCurrentRenderer(renderer);
}
