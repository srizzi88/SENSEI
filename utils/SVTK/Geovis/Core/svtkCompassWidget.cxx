/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompassWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkCompassWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkCompassRepresentation.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkCompassWidget);

//------------------------------------------------------------
svtkCompassWidget::svtkCompassWidget()
{
  // Set the initial state
  this->WidgetState = svtkCompassWidget::Start;
  this->TimerDuration = 50;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkCompassWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkCompassWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkCompassWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::TimerEvent, svtkWidgetEvent::TimedOut, this, svtkCompassWidget::TimerAction);
}

//----------------------------------------------------------------------
void svtkCompassWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkCompassRepresentation::New();
  }
}

//-----------------------------------------------------------------
double svtkCompassWidget::GetHeading()
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetHeading();
}

//-----------------------------------------------------------------
void svtkCompassWidget::SetHeading(double value)
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetHeading(value);
}

//-----------------------------------------------------------------
double svtkCompassWidget::GetTilt()
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetTilt();
}

//-----------------------------------------------------------------
void svtkCompassWidget::SetTilt(double value)
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetTilt(value);
}

//-----------------------------------------------------------------
double svtkCompassWidget::GetDistance()
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetDistance();
}

//-----------------------------------------------------------------
void svtkCompassWidget::SetDistance(double value)
{
  this->CreateDefaultRepresentation();
  svtkCompassRepresentation* slider = svtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetDistance(value);
}

//-------------------------------------------------------------
void svtkCompassWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkCompassWidget* self = reinterpret_cast<svtkCompassWidget*>(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(
      static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1])))
  {
    return;
  }

  // See if the widget has been selected. StartWidgetInteraction records the
  // starting point of the motion.
  self->CreateDefaultRepresentation();
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();

  if (interactionState == svtkCompassRepresentation::TiltDown)
  {
    self->SetTilt(self->GetTilt() - 15);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }
  if (interactionState == svtkCompassRepresentation::TiltUp)
  {
    self->SetTilt(self->GetTilt() + 15);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }

  if (interactionState == svtkCompassRepresentation::TiltAdjusting)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkCompassWidget::TiltAdjusting;
    // Start off the timer
    self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = svtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
  }

  if (interactionState == svtkCompassRepresentation::DistanceIn)
  {
    self->SetDistance(self->GetDistance() * 0.8);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }
  if (interactionState == svtkCompassRepresentation::DistanceOut)
  {
    self->SetDistance(self->GetDistance() * 1.2);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }

  if (interactionState == svtkCompassRepresentation::DistanceAdjusting)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkCompassWidget::DistanceAdjusting;
    // Start off the timer
    self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = svtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
  }

  if (interactionState != svtkCompassRepresentation::Adjusting)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->EventCallbackCommand->SetAbortFlag(1);
  if (interactionState == svtkCompassRepresentation::Adjusting)
  {
    self->WidgetState = svtkCompassWidget::Adjusting;
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
  }
}

//---------------------------------------------------------------
void svtkCompassWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkCompassWidget* self = reinterpret_cast<svtkCompassWidget*>(w);

  // do we need to change highlight state?
  self->CreateDefaultRepresentation();
  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);

  // if we are outside and in the start state then return
  if (interactionState == svtkCompassRepresentation::Outside &&
    self->WidgetState == svtkCompassWidget::Start)
  {
    return;
  }

  // if we are not outside and in the highlighting state then return
  if (interactionState != svtkCompassRepresentation::Outside &&
    self->WidgetState == svtkCompassWidget::Highlighting)
  {
    return;
  }

  // if we are not outside and in the Start state highlight
  if (interactionState != svtkCompassRepresentation::Outside &&
    self->WidgetState == svtkCompassWidget::Start)
  {
    self->WidgetRep->Highlight(1);
    self->WidgetState = svtkCompassWidget::Highlighting;
    self->Render();
    return;
  }

  // if we are outside but in the highlight state then stop highlighting
  if (self->WidgetState == svtkCompassWidget::Highlighting &&
    interactionState == svtkCompassRepresentation::Outside)
  {
    self->WidgetRep->Highlight(0);
    self->WidgetState = svtkCompassWidget::Start;
    self->Render();
    return;
  }

  svtkCompassRepresentation* rep = svtkCompassRepresentation::SafeDownCast(self->WidgetRep);

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  if (self->WidgetState == svtkCompassWidget::TiltAdjusting)
  {
    rep->TiltWidgetInteraction(eventPos);
  }
  if (self->WidgetState == svtkCompassWidget::DistanceAdjusting)
  {
    rep->DistanceWidgetInteraction(eventPos);
  }
  if (self->WidgetState == svtkCompassWidget::Adjusting)
  {
    self->WidgetRep->WidgetInteraction(eventPos);
  }
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-----------------------------------------------------------------
void svtkCompassWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkCompassWidget* self = reinterpret_cast<svtkCompassWidget*>(w);

  if (self->WidgetState != svtkCompassWidget::Adjusting &&
    self->WidgetState != svtkCompassWidget::TiltAdjusting &&
    self->WidgetState != svtkCompassWidget::DistanceAdjusting)
  {
    return;
  }

  if (self->WidgetState == svtkCompassWidget::TiltAdjusting)
  {
    // stop the timer
    self->Interactor->DestroyTimer(self->TimerId);
    svtkCompassRepresentation* rep = svtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    rep->EndTilt();
  }

  if (self->WidgetState == svtkCompassWidget::DistanceAdjusting)
  {
    // stop the timer
    self->Interactor->DestroyTimer(self->TimerId);
    svtkCompassRepresentation* rep = svtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    rep->EndDistance();
  }

  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);
  if (interactionState == svtkCompassRepresentation::Outside)
  {
    self->WidgetRep->Highlight(0);
    self->WidgetState = svtkCompassWidget::Start;
  }
  else
  {
    self->WidgetState = svtkCompassWidget::Highlighting;
  }

  // The state returns to unselected
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

void svtkCompassWidget::TimerAction(svtkAbstractWidget* w)
{
  svtkCompassWidget* self = reinterpret_cast<svtkCompassWidget*>(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if (timerId == self->TimerId)
  {
    svtkCompassRepresentation* rep = svtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    if (self->WidgetState == svtkCompassWidget::TiltAdjusting)
    {
      double tElapsed = svtkTimerLog::GetUniversalTime() - self->StartTime;
      rep->UpdateTilt(tElapsed);
    }
    if (self->WidgetState == svtkCompassWidget::DistanceAdjusting)
    {
      double tElapsed = svtkTimerLog::GetUniversalTime() - self->StartTime;
      rep->UpdateDistance(tElapsed);
    }
    self->StartTime = svtkTimerLog::GetUniversalTime();
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1); // no one else gets this timer
  }
}

//-----------------------------------------------------------------
void svtkCompassWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
