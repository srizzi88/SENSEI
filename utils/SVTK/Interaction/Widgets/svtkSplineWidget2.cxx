/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSplineWidget2.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSplineRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkSplineWidget2);
//----------------------------------------------------------------------------
svtkSplineWidget2::svtkSplineWidget2()
{
  this->WidgetState = svtkSplineWidget2::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkSplineWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkSplineWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkSplineWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkSplineWidget2::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkSplineWidget2::MoveAction);

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkSplineWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
svtkSplineWidget2::~svtkSplineWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkSplineWidget2::SetEnabled(int enabling)
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
void svtkSplineWidget2::SelectAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkSplineWidget2* self = svtkSplineWidget2::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkSplineWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkSplineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (interactionState == svtkSplineRepresentation::OnLine && self->Interactor->GetControlKey())
  {
    // Add point.
    reinterpret_cast<svtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkSplineRepresentation::Inserting);
  }
  else if (interactionState == svtkSplineRepresentation::OnHandle && self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<svtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkSplineRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<svtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkSplineRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSplineWidget2::TranslateAction(svtkAbstractWidget* w)
{
  // Not sure this should be any different that SelectAction
  svtkSplineWidget2::SelectAction(w);
}

//----------------------------------------------------------------------
void svtkSplineWidget2::ScaleAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkSplineWidget2* self = reinterpret_cast<svtkSplineWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkSplineWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkSplineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  // Scale
  reinterpret_cast<svtkSplineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkSplineRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSplineWidget2::MoveAction(svtkAbstractWidget* w)
{
  svtkSplineWidget2* self = reinterpret_cast<svtkSplineWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == svtkSplineWidget2::Start)
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
void svtkSplineWidget2::EndSelectAction(svtkAbstractWidget* w)
{
  svtkSplineWidget2* self = reinterpret_cast<svtkSplineWidget2*>(w);
  if (self->WidgetState == svtkSplineWidget2::Start)
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

  self->WidgetRep->EndWidgetInteraction(e);

  // Return state to not active
  self->WidgetState = svtkSplineWidget2::Start;
  reinterpret_cast<svtkSplineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkSplineRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSplineWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkSplineRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkSplineWidget2::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkSplineWidget2* self = static_cast<svtkSplineWidget2*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkSplineRepresentation* rep = svtkSplineRepresentation::SafeDownCast(self->WidgetRep);
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
void svtkSplineWidget2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
