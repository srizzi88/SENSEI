/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyle.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkEventForwarderCommand.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTDxInteractorStyleCamera.h"

svtkStandardNewMacro(svtkInteractorStyle);
svtkCxxSetObjectMacro(svtkInteractorStyle, TDxStyle, svtkTDxInteractorStyle);

//----------------------------------------------------------------------------
svtkInteractorStyle::svtkInteractorStyle()
{
  this->State = SVTKIS_NONE;
  this->AnimState = SVTKIS_ANIM_OFF;

  this->HandleObservers = 1;
  this->UseTimers = 0;
  this->TimerId = 1;

  this->AutoAdjustCameraClippingRange = 1;

  this->Interactor = nullptr;

  this->EventCallbackCommand->SetCallback(svtkInteractorStyle::ProcessEvents);

  // These widgets are not activated with a key

  this->KeyPressActivation = 0;

  this->Outline = svtkOutlineSource::New();
  this->OutlineActor = nullptr;
  this->OutlineMapper = svtkPolyDataMapper::New();

  if (this->OutlineMapper && this->Outline)
  {
    this->OutlineMapper->SetInputConnection(this->Outline->GetOutputPort());
  }

  this->PickedRenderer = nullptr;
  this->CurrentProp = nullptr;
  this->PropPicked = 0;

  this->PickColor[0] = 1.0;
  this->PickColor[1] = 0.0;
  this->PickColor[2] = 0.0;
  this->PickedActor2D = nullptr;

  this->MouseWheelMotionFactor = 1.0;

  this->TimerDuration = 10;
  this->EventForwarder = svtkEventForwarderCommand::New();

  this->TDxStyle = svtkTDxInteractorStyleCamera::New();
}

//----------------------------------------------------------------------------
svtkInteractorStyle::~svtkInteractorStyle()
{
  // Remove observers

  this->SetInteractor(nullptr);

  // Remove any highlight

  this->HighlightProp(nullptr);

  if (this->OutlineActor)
  {
    this->OutlineActor->Delete();
  }

  if (this->OutlineMapper)
  {
    this->OutlineMapper->Delete();
  }

  this->Outline->Delete();
  this->Outline = nullptr;

  this->SetCurrentRenderer(nullptr);
  this->EventForwarder->Delete();

  if (this->TDxStyle != nullptr)
  {
    this->TDxStyle->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //----------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling widget");

    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    this->Enabled = 1;
    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling-------------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;
    this->HighlightProp(nullptr);
    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
// NOTE!!! This does not do any reference counting!!!
// This is to avoid some ugly reference counting loops
// and the benefit of being able to hold only an entire
// renderwindow from an interactor style doesn't seem worth the
// mess.   Instead the svtkInteractorStyle sets up a DeleteEvent callback, so
// that it can tell when the svtkRenderWindowInteractor is going away.

void svtkInteractorStyle::SetInteractor(svtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
  {
    return;
  }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
  }
  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
  {
    i->AddObserver(svtkCommand::EnterEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::LeaveEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(
      svtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::RightButtonPressEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::MouseWheelForwardEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::MouseWheelBackwardEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::ExposeEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::ConfigureEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::TimerEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::KeyPressEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::KeyReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::CharEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::DeleteEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::TDxMotionEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::TDxButtonPressEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::TDxButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::StartSwipeEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::SwipeEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::EndSwipeEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::StartPinchEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::PinchEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::EndPinchEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::StartRotateEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RotateEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::EndRotateEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::StartPanEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::PanEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::EndPanEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::TapEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::LongTapEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::FourthButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(
      svtkCommand::FourthButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::FifthButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::FifthButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::Move3DEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::Button3DEvent, this->EventCallbackCommand, this->Priority);

    i->AddObserver(svtkCommand::DropFilesEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::UpdateDropLocationEvent, this->EventCallbackCommand, this->Priority);
  }

  this->EventForwarder->SetTarget(this->Interactor);
  if (this->Interactor)
  {
    this->AddObserver(svtkCommand::StartInteractionEvent, this->EventForwarder);
    this->AddObserver(svtkCommand::InteractionEvent, this->EventForwarder);
    this->AddObserver(svtkCommand::EndInteractionEvent, this->EventForwarder);
  }
  else
  {
    this->RemoveObserver(this->EventForwarder);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::FindPokedRenderer(int x, int y)
{
  this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(x, y));
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::HighlightProp(svtkProp* prop)
{
  this->CurrentProp = prop;

  if (prop != nullptr)
  {
    svtkActor2D* actor2D;
    svtkProp3D* prop3D;
    if ((prop3D = svtkProp3D::SafeDownCast(prop)) != nullptr)
    {
      this->HighlightProp3D(prop3D);
    }
    else if ((actor2D = svtkActor2D::SafeDownCast(prop)) != nullptr)
    {
      this->HighlightActor2D(actor2D);
    }
  }
  else
  { // unhighlight everything, both 2D & 3D
    this->HighlightProp3D(nullptr);
    this->HighlightActor2D(nullptr);
  }

  if (this->Interactor)
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
// When pick action successfully selects a svtkProp3Dactor, this method
// highlights the svtkProp3D appropriately. Currently this is done by placing a
// bounding box around the svtkProp3D.
void svtkInteractorStyle::HighlightProp3D(svtkProp3D* prop3D)
{
  // no prop picked now
  if (!prop3D)
  {
    // was there previously?
    if (this->PickedRenderer != nullptr && this->OutlineActor)
    {
      this->PickedRenderer->RemoveActor(this->OutlineActor);
      this->PickedRenderer = nullptr;
    }
  }
  // prop picked now
  else
  {
    if (!this->OutlineActor)
    {
      // have to defer creation to get right type
      this->OutlineActor = svtkActor::New();
      this->OutlineActor->PickableOff();
      this->OutlineActor->DragableOff();
      this->OutlineActor->SetMapper(this->OutlineMapper);
      this->OutlineActor->GetProperty()->SetColor(this->PickColor);
      this->OutlineActor->GetProperty()->SetAmbient(1.0);
      this->OutlineActor->GetProperty()->SetDiffuse(0.0);
    }

    // check if picked in different renderer to previous pick
    if (this->CurrentRenderer != this->PickedRenderer)
    {
      if (this->PickedRenderer != nullptr && this->OutlineActor)
      {
        this->PickedRenderer->RemoveActor(this->OutlineActor);
      }
      if (this->CurrentRenderer != nullptr)
      {
        this->CurrentRenderer->AddActor(this->OutlineActor);
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
      this->PickedRenderer = this->CurrentRenderer;
    }
    this->Outline->SetBounds(prop3D->GetBounds());
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::HighlightActor2D(svtkActor2D* actor2D)
{
  // If nothing has changed, just return
  if (actor2D == this->PickedActor2D)
  {
    return;
  }

  if (actor2D)
  {
    double tmpColor[3];
    actor2D->GetProperty()->GetColor(tmpColor);

    if (this->PickedActor2D)
    {
      actor2D->GetProperty()->SetColor(this->PickedActor2D->GetProperty()->GetColor());
      this->PickedActor2D->GetProperty()->SetColor(this->PickColor);
    }
    else
    {
      actor2D->GetProperty()->SetColor(this->PickColor);
    }

    this->PickColor[0] = tmpColor[0];
    this->PickColor[1] = tmpColor[1];
    this->PickColor[2] = tmpColor[2];
  }
  else
  {
    if (this->PickedActor2D)
    {
      double tmpColor[3];
      this->PickedActor2D->GetProperty()->GetColor(tmpColor);
      this->PickedActor2D->GetProperty()->SetColor(this->PickColor);
      this->PickColor[0] = tmpColor[0];
      this->PickColor[1] = tmpColor[1];
      this->PickColor[2] = tmpColor[2];
    }
  }

  this->PickedActor2D = actor2D;
}

//----------------------------------------------------------------------------
// Implementation of motion state control methods
//----------------------------------------------------------------------------
void svtkInteractorStyle::StartState(int newstate)
{
  this->State = newstate;
  if (this->AnimState == SVTKIS_ANIM_OFF)
  {
    svtkRenderWindowInteractor* rwi = this->Interactor;
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
    this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    if (this->UseTimers && !(this->TimerId = rwi->CreateRepeatingTimer(this->TimerDuration)))
    {
      // FIXME: This comment doesn't match the logic of the code. There is one
      // test failing with it like it is, but more failing if the comparison is
      // inverted.
      // svtkTestingInteractor cannot create timers
      if (std::string(rwi->GetClassName()) != "svtkTestingInteractor")
      {
        svtkErrorMacro(<< "Timer start failed");
      }
      this->State = SVTKIS_NONE;
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StopState()
{
  this->State = SVTKIS_NONE;
  if (this->AnimState == SVTKIS_ANIM_OFF)
  {
    svtkRenderWindowInteractor* rwi = this->Interactor;
    svtkRenderWindow* renwin = rwi->GetRenderWindow();
    renwin->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
    if (this->UseTimers &&
      // FIXME: This comment doesn't match the logic of the code. There is
      // one test failing with it like it is, but more failing if the
      // comparison is inverted.
      // svtkTestingInteractor cannot create timers
      std::string(rwi->GetClassName()) != "svtkTestingInteractor" &&
      !rwi->DestroyTimer(this->TimerId))
    {
      svtkErrorMacro(<< "Timer stop failed");
    }
    this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
    rwi->Render();
  }
}

//----------------------------------------------------------------------------
// JCP animation control
void svtkInteractorStyle::StartAnimate()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;
  this->AnimState = SVTKIS_ANIM_ON;
  if (this->State == SVTKIS_NONE)
  {
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
    if (this->UseTimers && !(this->TimerId = rwi->CreateRepeatingTimer(this->TimerDuration)))
    {
      svtkErrorMacro(<< "Timer start failed");
    }
  }
  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StopAnimate()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;
  this->AnimState = SVTKIS_ANIM_OFF;
  if (this->State == SVTKIS_NONE)
  {
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
    if (this->UseTimers && !rwi->DestroyTimer(this->TimerId))
    {
      svtkErrorMacro(<< "Timer stop failed");
    }
  }
}

// JCP Animation control
//----------------------------------------------------------------------------
void svtkInteractorStyle::StartRotate()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_ROTATE);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndRotate()
{
  if (this->State != SVTKIS_ROTATE)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartZoom()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_ZOOM);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndZoom()
{
  if (this->State != SVTKIS_ZOOM)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartPan()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_PAN);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndPan()
{
  if (this->State != SVTKIS_PAN)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartSpin()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_SPIN);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndSpin()
{
  if (this->State != SVTKIS_SPIN)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartDolly()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_DOLLY);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndDolly()
{
  if (this->State != SVTKIS_DOLLY)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartUniformScale()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_USCALE);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndUniformScale()
{
  if (this->State != SVTKIS_USCALE)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartTimer()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_TIMER);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndTimer()
{
  if (this->State != SVTKIS_TIMER)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartTwoPointer()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_TWO_POINTER);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndTwoPointer()
{
  if (this->State != SVTKIS_TWO_POINTER)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartGesture()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_GESTURE);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndGesture()
{
  if (this->State != SVTKIS_GESTURE)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::StartEnvRotate()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_ENV_ROTATE);
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::EndEnvRotate()
{
  if (this->State != SVTKIS_ENV_ROTATE)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
// By overriding the Rotate, Rotate members we can
// use this timer routine for Joystick or Trackball - quite tidy
//----------------------------------------------------------------------------
void svtkInteractorStyle::OnTimer()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;

  switch (this->State)
  {
    case SVTKIS_NONE:
      if (this->AnimState == SVTKIS_ANIM_ON)
      {
        if (this->UseTimers)
        {
          rwi->DestroyTimer(this->TimerId);
        }
        rwi->Render();
        if (this->UseTimers)
        {
          this->TimerId = rwi->CreateRepeatingTimer(this->TimerDuration);
        }
      }
      break;

    case SVTKIS_ROTATE:
      this->Rotate();
      break;

    case SVTKIS_PAN:
      this->Pan();
      break;

    case SVTKIS_SPIN:
      this->Spin();
      break;

    case SVTKIS_DOLLY:
      this->Dolly();
      break;

    case SVTKIS_ZOOM:
      this->Zoom();
      break;

    case SVTKIS_USCALE:
      this->UniformScale();
      break;

    case SVTKIS_ENV_ROTATE:
      this->EnvironmentRotate();
      break;

    case SVTKIS_TIMER:
      rwi->Render();
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::OnChar()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;

  switch (rwi->GetKeyCode())
  {
    case 'm':
    case 'M':
      if (this->AnimState == SVTKIS_ANIM_OFF)
      {
        this->StartAnimate();
      }
      else
      {
        this->StopAnimate();
      }
      break;

    case 'Q':
    case 'q':
    case 'e':
    case 'E':
      rwi->ExitCallback();
      break;

    case 'f':
    case 'F':
    {
      if (this->CurrentRenderer != nullptr)
      {
        this->AnimState = SVTKIS_ANIM_ON;
        svtkAssemblyPath* path = nullptr;
        this->FindPokedRenderer(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]);
        rwi->GetPicker()->Pick(
          rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], 0.0, this->CurrentRenderer);
        svtkAbstractPropPicker* picker;
        if ((picker = svtkAbstractPropPicker::SafeDownCast(rwi->GetPicker())))
        {
          path = picker->GetPath();
        }
        if (path != nullptr)
        {
          rwi->FlyTo(this->CurrentRenderer, picker->GetPickPosition());
        }
        this->AnimState = SVTKIS_ANIM_OFF;
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
    }
    break;

    case 'u':
    case 'U':
      rwi->UserCallback();
      break;

    case 'r':
    case 'R':
      this->FindPokedRenderer(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]);
      if (this->CurrentRenderer != nullptr)
      {
        this->CurrentRenderer->ResetCamera();
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
      rwi->Render();
      break;

    case 'w':
    case 'W':
    {
      svtkActorCollection* ac;
      svtkActor *anActor, *aPart;
      svtkAssemblyPath* path;
      this->FindPokedRenderer(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]);
      if (this->CurrentRenderer != nullptr)
      {
        ac = this->CurrentRenderer->GetActors();
        svtkCollectionSimpleIterator ait;
        for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
        {
          for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
          {
            aPart = static_cast<svtkActor*>(path->GetLastNode()->GetViewProp());
            aPart->GetProperty()->SetRepresentationToWireframe();
          }
        }
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
      rwi->Render();
    }
    break;

    case 's':
    case 'S':
    {
      svtkActorCollection* ac;
      svtkActor *anActor, *aPart;
      svtkAssemblyPath* path;
      this->FindPokedRenderer(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]);
      if (this->CurrentRenderer != nullptr)
      {
        ac = this->CurrentRenderer->GetActors();
        svtkCollectionSimpleIterator ait;
        for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
        {
          for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
          {
            aPart = static_cast<svtkActor*>(path->GetLastNode()->GetViewProp());
            aPart->GetProperty()->SetRepresentationToSurface();
          }
        }
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
      rwi->Render();
    }
    break;

    case '3':
      if (rwi->GetRenderWindow()->GetStereoRender())
      {
        rwi->GetRenderWindow()->StereoRenderOff();
      }
      else
      {
        rwi->GetRenderWindow()->StereoRenderOn();
      }
      rwi->Render();
      break;

    case 'p':
    case 'P':
      if (this->CurrentRenderer != nullptr)
      {
        if (this->State == SVTKIS_NONE)
        {
          svtkAssemblyPath* path = nullptr;
          int* eventPos = rwi->GetEventPosition();
          this->FindPokedRenderer(eventPos[0], eventPos[1]);
          rwi->StartPickCallback();
          svtkAbstractPropPicker* picker = svtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
          if (picker != nullptr)
          {
            picker->Pick(eventPos[0], eventPos[1], 0.0, this->CurrentRenderer);
            path = picker->GetPath();
          }
          if (path == nullptr)
          {
            this->HighlightProp(nullptr);
            this->PropPicked = 0;
          }
          else
          {
            this->HighlightProp(path->GetFirstNode()->GetViewProp());
            this->PropPicked = 1;
          }
          rwi->EndPickCallback();
        }
      }
      else
      {
        svtkWarningMacro(<< "no current renderer on the interactor style.");
      }
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Auto Adjust Camera Clipping Range "
     << (this->AutoAdjustCameraClippingRange ? "On\n" : "Off\n");

  os << indent << "Pick Color: (" << this->PickColor[0] << ", " << this->PickColor[1] << ", "
     << this->PickColor[2] << ")\n";

  os << indent << "CurrentRenderer: " << this->CurrentRenderer << "\n";
  if (this->PickedRenderer)
  {
    os << indent << "Picked Renderer: " << this->PickedRenderer << "\n";
  }
  else
  {
    os << indent << "Picked Renderer: (none)\n";
  }
  if (this->CurrentProp)
  {
    os << indent << "Current Prop: " << this->CurrentProp << "\n";
  }
  else
  {
    os << indent << "Current Actor: (none)\n";
  }

  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Prop Picked: " << (this->PropPicked ? "Yes\n" : "No\n");

  os << indent << "State: " << this->State << endl;
  os << indent << "UseTimers: " << this->UseTimers << endl;
  os << indent << "HandleObservers: " << this->HandleObservers << endl;
  os << indent << "MouseWheelMotionFactor: " << this->MouseWheelMotionFactor << endl;

  os << indent << "Timer Duration: " << this->TimerDuration << endl;

  os << indent << "TDxStyle: ";
  if (this->TDxStyle == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    this->TDxStyle->PrintSelf(os, indent.GetNextIndent());
  }
}

// ----------------------------------------------------------------------------
void svtkInteractorStyle::DelegateTDxEvent(unsigned long event, void* calldata)
{
  if (this->TDxStyle != nullptr)
  {
    this->TDxStyle->ProcessEvent(this->CurrentRenderer, event, calldata);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyle::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* calldata)
{
  svtkInteractorStyle* self = reinterpret_cast<svtkInteractorStyle*>(clientdata);

  bool aborted = false;

  switch (event)
  {
    case svtkCommand::ExposeEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::ExposeEvent))
      {
        self->InvokeEvent(svtkCommand::ExposeEvent, nullptr);
      }
      else
      {
        self->OnExpose();
      }
      break;

    case svtkCommand::ConfigureEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::ConfigureEvent))
      {
        self->InvokeEvent(svtkCommand::ConfigureEvent, nullptr);
      }
      else
      {
        self->OnConfigure();
      }
      break;

    case svtkCommand::EnterEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::EnterEvent))
      {
        self->InvokeEvent(svtkCommand::EnterEvent, nullptr);
      }
      else
      {
        self->OnEnter();
      }
      break;

    case svtkCommand::LeaveEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::LeaveEvent))
      {
        self->InvokeEvent(svtkCommand::LeaveEvent, nullptr);
      }
      else
      {
        self->OnLeave();
      }
      break;

    case svtkCommand::TimerEvent:
    {
      // The calldata should be a timer id, but because of legacy we check
      // and make sure that it is non-nullptr.
      int timerId = (calldata ? *(reinterpret_cast<int*>(calldata)) : 1);
      if (self->HandleObservers && self->HasObserver(svtkCommand::TimerEvent))
      {
        self->InvokeEvent(svtkCommand::TimerEvent, &timerId);
      }
      else
      {
        self->OnTimer();
      }
    }
    break;

    case svtkCommand::MouseMoveEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::MouseMoveEvent))
      {
        self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);
      }
      else
      {
        self->OnMouseMove();
      }
      break;

    case svtkCommand::LeftButtonPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::LeftButtonPressEvent))
      {
        self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
      }
      else
      {
        self->OnLeftButtonDown();
      }
      break;

    case svtkCommand::LeftButtonReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::LeftButtonReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
      }
      else
      {
        self->OnLeftButtonUp();
      }
      break;

    case svtkCommand::MiddleButtonPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::MiddleButtonPressEvent))
      {
        self->InvokeEvent(svtkCommand::MiddleButtonPressEvent, nullptr);
      }
      else
      {
        self->OnMiddleButtonDown();
      }
      break;

    case svtkCommand::MiddleButtonReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::MiddleButtonReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::MiddleButtonReleaseEvent, nullptr);
      }
      else
      {
        self->OnMiddleButtonUp();
      }
      break;

    case svtkCommand::RightButtonPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::RightButtonPressEvent))
      {
        self->InvokeEvent(svtkCommand::RightButtonPressEvent, nullptr);
      }
      else
      {
        self->OnRightButtonDown();
      }
      break;

    case svtkCommand::RightButtonReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::RightButtonReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::RightButtonReleaseEvent, nullptr);
      }
      else
      {
        self->OnRightButtonUp();
      }
      break;

    case svtkCommand::MouseWheelForwardEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::MouseWheelForwardEvent))
      {
        self->InvokeEvent(svtkCommand::MouseWheelForwardEvent, nullptr);
      }
      else
      {
        self->OnMouseWheelForward();
      }
      break;

    case svtkCommand::MouseWheelBackwardEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::MouseWheelBackwardEvent))
      {
        self->InvokeEvent(svtkCommand::MouseWheelBackwardEvent, nullptr);
      }
      else
      {
        self->OnMouseWheelBackward();
      }
      break;

    case svtkCommand::KeyPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::KeyPressEvent))
      {
        self->InvokeEvent(svtkCommand::KeyPressEvent, nullptr);
      }
      else
      {
        self->OnKeyDown();
        self->OnKeyPress();
      }
      break;

    case svtkCommand::KeyReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::KeyReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::KeyReleaseEvent, nullptr);
      }
      else
      {
        self->OnKeyUp();
        self->OnKeyRelease();
      }
      break;

    case svtkCommand::CharEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::CharEvent))
      {
        self->InvokeEvent(svtkCommand::CharEvent, nullptr);
      }
      else
      {
        self->OnChar();
      }
      break;

    case svtkCommand::DeleteEvent:
      self->SetInteractor(nullptr);
      break;

    case svtkCommand::TDxMotionEvent:
    case svtkCommand::TDxButtonPressEvent:
    case svtkCommand::TDxButtonReleaseEvent:
      self->DelegateTDxEvent(event, calldata);
      break;

    case svtkCommand::StartSwipeEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::StartSwipeEvent))
      {
        self->InvokeEvent(svtkCommand::StartSwipeEvent, nullptr);
      }
      else
      {
        self->OnStartSwipe();
      }
      break;
    case svtkCommand::SwipeEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::SwipeEvent))
      {
        self->InvokeEvent(svtkCommand::SwipeEvent, nullptr);
      }
      else
      {
        self->OnSwipe();
      }
      break;
    case svtkCommand::EndSwipeEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::EndSwipeEvent))
      {
        self->InvokeEvent(svtkCommand::EndSwipeEvent, nullptr);
      }
      else
      {
        self->OnEndSwipe();
      }
      break;

    case svtkCommand::StartPinchEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::StartPinchEvent))
      {
        self->InvokeEvent(svtkCommand::StartPinchEvent, nullptr);
      }
      else
      {
        self->OnStartPinch();
      }
      break;
    case svtkCommand::PinchEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::PinchEvent))
      {
        self->InvokeEvent(svtkCommand::PinchEvent, nullptr);
      }
      else
      {
        self->OnPinch();
      }
      break;
    case svtkCommand::EndPinchEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::EndPinchEvent))
      {
        self->InvokeEvent(svtkCommand::EndPinchEvent, nullptr);
      }
      else
      {
        self->OnEndPinch();
      }
      break;

    case svtkCommand::StartPanEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::StartPanEvent))
      {
        self->InvokeEvent(svtkCommand::StartPanEvent, nullptr);
      }
      else
      {
        self->OnStartPan();
      }
      break;
    case svtkCommand::PanEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::PanEvent))
      {
        self->InvokeEvent(svtkCommand::PanEvent, nullptr);
      }
      else
      {
        self->OnPan();
      }
      break;
    case svtkCommand::EndPanEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::EndPanEvent))
      {
        self->InvokeEvent(svtkCommand::EndPanEvent, nullptr);
      }
      else
      {
        self->OnEndPan();
      }
      break;

    case svtkCommand::StartRotateEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::StartRotateEvent))
      {
        self->InvokeEvent(svtkCommand::StartRotateEvent, nullptr);
      }
      else
      {
        self->OnStartRotate();
      }
      break;
    case svtkCommand::RotateEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::RotateEvent))
      {
        self->InvokeEvent(svtkCommand::RotateEvent, nullptr);
      }
      else
      {
        self->OnRotate();
      }
      break;
    case svtkCommand::EndRotateEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::EndRotateEvent))
      {
        self->InvokeEvent(svtkCommand::EndRotateEvent, nullptr);
      }
      else
      {
        self->OnEndRotate();
      }
      break;

    case svtkCommand::TapEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::TapEvent))
      {
        self->InvokeEvent(svtkCommand::TapEvent, nullptr);
      }
      else
      {
        self->OnTap();
      }
      break;

    case svtkCommand::LongTapEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::LongTapEvent))
      {
        self->InvokeEvent(svtkCommand::LongTapEvent, nullptr);
      }
      else
      {
        self->OnLongTap();
      }
      break;

    case svtkCommand::FourthButtonPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::FourthButtonPressEvent))
      {
        self->InvokeEvent(svtkCommand::FourthButtonPressEvent, nullptr);
      }
      else
      {
        self->OnFourthButtonDown();
      }
      break;

    case svtkCommand::FourthButtonReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::FourthButtonReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::FourthButtonReleaseEvent, nullptr);
      }
      else
      {
        self->OnFourthButtonUp();
      }
      break;

    case svtkCommand::FifthButtonPressEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::FifthButtonPressEvent))
      {
        self->InvokeEvent(svtkCommand::FifthButtonPressEvent, nullptr);
      }
      else
      {
        self->OnFifthButtonDown();
      }
      break;

    case svtkCommand::FifthButtonReleaseEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::FifthButtonReleaseEvent))
      {
        self->InvokeEvent(svtkCommand::FifthButtonReleaseEvent, nullptr);
      }
      else
      {
        self->OnFifthButtonUp();
      }
      break;

    case svtkCommand::Move3DEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::Move3DEvent))
      {
        aborted = (self->InvokeEvent(svtkCommand::Move3DEvent, calldata) == 1);
      }
      if (!aborted)
      {
        self->OnMove3D(static_cast<svtkEventData*>(calldata));
      }
      break;

    case svtkCommand::Button3DEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::Button3DEvent))
      {
        aborted = (self->InvokeEvent(svtkCommand::Button3DEvent, calldata) == 1);
      }
      if (!aborted)
      {
        self->OnButton3D(static_cast<svtkEventData*>(calldata));
      }
      break;

    case svtkCommand::DropFilesEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::DropFilesEvent))
      {
        aborted = (self->InvokeEvent(svtkCommand::DropFilesEvent, calldata) == 1);
      }
      if (!aborted)
      {
        self->OnDropFiles(static_cast<svtkStringArray*>(calldata));
      }
      break;

    case svtkCommand::UpdateDropLocationEvent:
      if (self->HandleObservers && self->HasObserver(svtkCommand::UpdateDropLocationEvent))
      {
        aborted = (self->InvokeEvent(svtkCommand::UpdateDropLocationEvent, calldata) == 1);
      }
      if (!aborted)
      {
        self->OnDropLocation(static_cast<double*>(calldata));
      }
      break;
  }
}
