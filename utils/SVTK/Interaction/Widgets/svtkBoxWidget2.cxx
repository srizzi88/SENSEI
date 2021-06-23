/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBoxWidget2.h"
#include "svtkBoxRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkEventData.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkBoxWidget2);

//----------------------------------------------------------------------------
svtkBoxWidget2::svtkBoxWidget2()
{
  this->WidgetState = svtkBoxWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = true;
  this->ScalingEnabled = true;
  this->RotationEnabled = true;
  this->MoveFacesEnabled = true;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkEvent::NoModifier, 0,
    0, nullptr, svtkWidgetEvent::Select, this, svtkBoxWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent, svtkEvent::NoModifier,
    0, 0, nullptr, svtkWidgetEvent::EndSelect, this, svtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkEvent::ControlModifier, 0, 0, nullptr, svtkWidgetEvent::Translate, this,
    svtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkEvent::ControlModifier, 0, 0, nullptr, svtkWidgetEvent::EndTranslate, this,
    svtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkEvent::ShiftModifier,
    0, 0, nullptr, svtkWidgetEvent::Translate, this, svtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkEvent::ShiftModifier, 0, 0, nullptr, svtkWidgetEvent::EndTranslate, this,
    svtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkBoxWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkBoxWidget2::MoveAction);

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed.Get(),
      svtkWidgetEvent::Select3D, this, svtkBoxWidget2::SelectAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed.Get(),
      svtkWidgetEvent::EndSelect3D, this, svtkBoxWidget2::EndSelectAction3D);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      svtkCommand::Move3DEvent, ed.Get(), svtkWidgetEvent::Move3D, this, svtkBoxWidget2::MoveAction3D);
  }

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkBoxWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
svtkBoxWidget2::~svtkBoxWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkBoxWidget2::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    if (this->Parent)
    {
      this->Parent->AddObserver(
        svtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Parent->AddObserver(
        svtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
    else
    {
      this->Interactor->AddObserver(
        svtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Interactor->AddObserver(
        svtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
  }
  else if (!enabling && enabled)
  {
    if (this->Parent)
    {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
    }
    else
    {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
    }
  }
}

//----------------------------------------------------------------------
void svtkBoxWidget2::SelectAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == svtkBoxRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == svtkBoxRepresentation::MoveF0 ||
        interactionState == svtkBoxRepresentation::MoveF1 ||
        interactionState == svtkBoxRepresentation::MoveF2 ||
        interactionState == svtkBoxRepresentation::MoveF3 ||
        interactionState == svtkBoxRepresentation::MoveF4 ||
        interactionState == svtkBoxRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == svtkBoxRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  reinterpret_cast<svtkBoxRepresentation*>(self->WidgetRep)->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkBoxWidget2::SelectAction3D(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == svtkBoxRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == svtkBoxRepresentation::MoveF0 ||
        interactionState == svtkBoxRepresentation::MoveF1 ||
        interactionState == svtkBoxRepresentation::MoveF2 ||
        interactionState == svtkBoxRepresentation::MoveF3 ||
        interactionState == svtkBoxRepresentation::MoveF4 ||
        interactionState == svtkBoxRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == svtkBoxRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = svtkBoxWidget2::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBoxWidget2::TranslateAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  if (self->TranslationEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<svtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkBoxRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkBoxWidget2::ScaleAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  if (self->ScalingEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<svtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkBoxRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkBoxWidget2::MoveAction(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == svtkBoxWidget2::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkBoxWidget2::MoveAction3D(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == svtkBoxWidget2::Start)
  {
    return;
  }

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBoxWidget2::EndSelectAction(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);
  if (self->WidgetState == svtkBoxWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = svtkBoxWidget2::Start;
  reinterpret_cast<svtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkBoxRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkBoxWidget2::EndSelectAction3D(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  if (self->WidgetState != svtkBoxWidget2::Active ||
    self->WidgetRep->GetInteractionState() == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = svtkBoxWidget2::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBoxWidget2::StepAction3D(svtkAbstractWidget* w)
{
  svtkBoxWidget2* self = reinterpret_cast<svtkBoxWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == svtkBoxRepresentation::Outside)
  {
    return;
  }

  // self->WidgetRep->SetInteractionState(svtkBoxRepresentation::Outside);

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBoxWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkBoxRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkBoxWidget2::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkBoxWidget2* self = static_cast<svtkBoxWidget2*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkBoxRepresentation* rep = svtkBoxRepresentation::SafeDownCast(self->WidgetRep);
  switch (event)
  {
    case svtkCommand::KeyPressEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
          rep->SetXTranslationAxisOn();
          break;
        case 'y':
        case 'Y':
          rep->SetYTranslationAxisOn();
          break;
        case 'z':
        case 'Z':
          rep->SetZTranslationAxisOn();
          break;
        default:
          break;
      }
      break;
    case svtkCommand::KeyReleaseEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
        case 'y':
        case 'Y':
        case 'z':
        case 'Z':
          rep->SetTranslationAxisOff();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkBoxWidget2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
  os << indent << "Move Faces Enabled: " << (this->MoveFacesEnabled ? "On\n" : "Off\n");
}
