/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSphereWidget2.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkSphereWidget2);

//----------------------------------------------------------------------------
svtkSphereWidget2::svtkSphereWidget2()
{
  this->WidgetState = svtkSphereWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = 1;
  this->ScalingEnabled = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkSphereWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkSphereWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkSphereWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkSphereWidget2::MoveAction);

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkSphereWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
svtkSphereWidget2::~svtkSphereWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkSphereWidget2::SetEnabled(int enabling)
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
void svtkSphereWidget2::SelectAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkSphereWidget2* self = reinterpret_cast<svtkSphereWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // Modifier keys force us into translare mode
  // The SetInteractionState has the side effect of highlighting the widget
  if (interactionState == svtkSphereRepresentation::OnSphere || self->Interactor->GetShiftKey() ||
    self->Interactor->GetControlKey())
  {
    // If translation is disabled, do it
    if (self->TranslationEnabled)
    {
      reinterpret_cast<svtkSphereRepresentation*>(self->WidgetRep)
        ->SetInteractionState(svtkSphereRepresentation::Translating);
    }
  }
  else
  {
    reinterpret_cast<svtkSphereRepresentation*>(self->WidgetRep)
      ->SetInteractionState(interactionState);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSphereWidget2::TranslateAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkSphereWidget2* self = reinterpret_cast<svtkSphereWidget2*>(w);

  // If  translation is disabled, get out of here
  if (!self->TranslationEnabled)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<svtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkSphereRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSphereWidget2::ScaleAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkSphereWidget2* self = reinterpret_cast<svtkSphereWidget2*>(w);

  // If scaling is disabled, get out of here
  if (!self->ScalingEnabled)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkSphereWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSphereRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<svtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkSphereRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSphereWidget2::MoveAction(svtkAbstractWidget* w)
{
  svtkSphereWidget2* self = reinterpret_cast<svtkSphereWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == svtkSphereWidget2::Start)
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
void svtkSphereWidget2::EndSelectAction(svtkAbstractWidget* w)
{
  svtkSphereWidget2* self = reinterpret_cast<svtkSphereWidget2*>(w);
  if (self->WidgetState == svtkSphereWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = svtkSphereWidget2::Start;
  reinterpret_cast<svtkSphereRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkSphereRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSphereWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkSphereRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkSphereWidget2::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkSphereWidget2* self = static_cast<svtkSphereWidget2*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkSphereRepresentation* rep = svtkSphereRepresentation::SafeDownCast(self->WidgetRep);
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
void svtkSphereWidget2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
}
