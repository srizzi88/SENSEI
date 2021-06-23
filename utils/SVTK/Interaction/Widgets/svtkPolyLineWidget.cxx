/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyLineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyLineWidget.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkPolyLineRepresentation.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkPolyLineWidget);
//----------------------------------------------------------------------------
svtkPolyLineWidget::svtkPolyLineWidget()
{
  this->WidgetState = svtkPolyLineWidget::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkPolyLineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkPolyLineWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkPolyLineWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkPolyLineWidget::MoveAction);

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkPolyLineWidget::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
svtkPolyLineWidget::~svtkPolyLineWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkPolyLineWidget::SetEnabled(int enabling)
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
void svtkPolyLineWidget::SelectAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkPolyLineWidget* self = svtkPolyLineWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkPolyLineWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkPolyLineRepresentation::Outside && !self->Interactor->GetAltKey())
  {
    return;
  }
  // We are definitely selected
  self->WidgetState = svtkPolyLineWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (self->Interactor->GetAltKey())
  {
    // push point.
    reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkPolyLineRepresentation::Pushing);
  }
  else if (interactionState == svtkPolyLineRepresentation::OnLine &&
    self->Interactor->GetControlKey())
  {
    // insert point.
    reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkPolyLineRepresentation::Inserting);
  }
  else if (interactionState == svtkPolyLineRepresentation::OnHandle &&
    self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkPolyLineRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkPolyLineRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkPolyLineWidget::TranslateAction(svtkAbstractWidget* w)
{
  // Not sure this should be any different than SelectAction
  svtkPolyLineWidget::SelectAction(w);
}

//----------------------------------------------------------------------
void svtkPolyLineWidget::ScaleAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkPolyLineWidget* self = reinterpret_cast<svtkPolyLineWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkPolyLineWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkPolyLineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkPolyLineWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  // Scale
  reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkPolyLineRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkPolyLineWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkPolyLineWidget* self = reinterpret_cast<svtkPolyLineWidget*>(w);

  // See whether we're active
  if (self->WidgetState == svtkPolyLineWidget::Start)
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
void svtkPolyLineWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkPolyLineWidget* self = reinterpret_cast<svtkPolyLineWidget*>(w);
  if (self->WidgetState == svtkPolyLineWidget::Start)
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

  // EndWidgetInteraction for this widget can modify/add/remove points
  // Make sure the representation is updated
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);

  // Return state to not active
  self->WidgetState = svtkPolyLineWidget::Start;
  reinterpret_cast<svtkPolyLineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkPolyLineRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------
void svtkPolyLineWidget::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkPolyLineWidget* self = static_cast<svtkPolyLineWidget*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkPolyLineRepresentation* rep = svtkPolyLineRepresentation::SafeDownCast(self->WidgetRep);
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

//----------------------------------------------------------------------
void svtkPolyLineWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkPolyLineRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkPolyLineWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
