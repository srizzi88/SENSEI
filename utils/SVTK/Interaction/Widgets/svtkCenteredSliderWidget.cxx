/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCenteredSliderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCenteredSliderWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliderRepresentation2D.h"
#include "svtkTimerLog.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkCenteredSliderWidget);

//------------------------------------------------------------
svtkCenteredSliderWidget::svtkCenteredSliderWidget()
{
  // Set the initial state
  this->WidgetState = svtkCenteredSliderWidget::Start;
  this->TimerDuration = 50;
  this->Value = 0;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkCenteredSliderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkCenteredSliderWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkCenteredSliderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::TimerEvent, svtkWidgetEvent::TimedOut, this, svtkCenteredSliderWidget::TimerAction);
}

//----------------------------------------------------------------------
void svtkCenteredSliderWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkSliderRepresentation2D::New();
  }
}

//-------------------------------------------------------------
void svtkCenteredSliderWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkCenteredSliderWidget* self = svtkCenteredSliderWidget::SafeDownCast(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(
      static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1])))
  {
    self->WidgetState = svtkCenteredSliderWidget::Start;
    return;
  }

  // See if the widget has been selected. StartWidgetInteraction records the
  // starting point of the motion.
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == svtkSliderRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  svtkSliderRepresentation* slider = svtkSliderRepresentation::SafeDownCast(self->WidgetRep);
  self->EventCallbackCommand->SetAbortFlag(1);
  if (interactionState == svtkSliderRepresentation::Slider)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkCenteredSliderWidget::Sliding;
    // Start off the timer
    self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = svtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->Render();
    return;
  }

  if (interactionState == svtkSliderRepresentation::LeftCap)
  {
    self->Value = slider->GetMinimumValue();
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->Render();
    return;
  }
  if (interactionState == svtkSliderRepresentation::RightCap)
  {
    self->Value = slider->GetMaximumValue();
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->Render();
    return;
  }
}

//---------------------------------------------------------------
void svtkCenteredSliderWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkCenteredSliderWidget* self = svtkCenteredSliderWidget::SafeDownCast(w);

  // See whether we're active
  if (self->WidgetState == svtkCenteredSliderWidget::Start)
  {
    return;
  }

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->WidgetInteraction(eventPos);

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-----------------------------------------------------------------
void svtkCenteredSliderWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkCenteredSliderWidget* self = svtkCenteredSliderWidget::SafeDownCast(w);

  if (self->WidgetState == svtkCenteredSliderWidget::Start)
  {
    return;
  }

  // stop the timer
  self->Interactor->DestroyTimer(self->TimerId);

  // Highlight if necessary
  svtkSliderRepresentation* slider = svtkSliderRepresentation::SafeDownCast(self->WidgetRep);
  slider->SetValue((slider->GetMinimumValue() + slider->GetMaximumValue()) / 2.0);
  self->WidgetRep->Highlight(0);

  // The state returns to unselected
  self->WidgetState = svtkCenteredSliderWidget::Start;
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

void svtkCenteredSliderWidget::TimerAction(svtkAbstractWidget* w)
{
  svtkCenteredSliderWidget* self = svtkCenteredSliderWidget::SafeDownCast(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if (timerId == self->TimerId && self->WidgetState == svtkCenteredSliderWidget::Sliding)
  {
    self->Value = svtkTimerLog::GetUniversalTime() - self->StartTime;

    svtkSliderRepresentation* slider = svtkSliderRepresentation::SafeDownCast(self->WidgetRep);
    double avg = (slider->GetMinimumValue() + slider->GetMaximumValue()) / 2.0;
    self->Value = avg + (slider->GetValue() - avg) * self->Value;
    self->StartTime = svtkTimerLog::GetUniversalTime();
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1); // no one else gets this timer
    self->Render();
  }
}

//-----------------------------------------------------------------
void svtkCenteredSliderWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
