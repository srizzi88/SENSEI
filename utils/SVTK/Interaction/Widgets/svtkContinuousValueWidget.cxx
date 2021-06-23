/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContinuousValueWidget.cxx

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

#include "svtkContinuousValueWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkContinuousValueWidgetRepresentation.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

//------------------------------------------------------------
svtkContinuousValueWidget::svtkContinuousValueWidget()
{
  // Set the initial state
  this->WidgetState = svtkContinuousValueWidget::Start;
  this->Value = 0;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkContinuousValueWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkContinuousValueWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkContinuousValueWidget::EndSelectAction);
}

//-----------------------------------------------------------------
double svtkContinuousValueWidget::GetValue()
{
  svtkContinuousValueWidgetRepresentation* slider =
    svtkContinuousValueWidgetRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetValue();
}

//-----------------------------------------------------------------
void svtkContinuousValueWidget::SetValue(double value)
{
  svtkContinuousValueWidgetRepresentation* slider =
    svtkContinuousValueWidgetRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetValue(value);
}

//-------------------------------------------------------------
void svtkContinuousValueWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkContinuousValueWidget* self = reinterpret_cast<svtkContinuousValueWidget*>(w);

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
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState != svtkContinuousValueWidgetRepresentation::Adjusting)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->EventCallbackCommand->SetAbortFlag(1);
  if (interactionState == svtkContinuousValueWidgetRepresentation::Adjusting)
  {
    self->WidgetState = svtkContinuousValueWidget::Adjusting;
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->Render();
    return;
  }
}

//---------------------------------------------------------------
void svtkContinuousValueWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkContinuousValueWidget* self = reinterpret_cast<svtkContinuousValueWidget*>(w);

  // do we need to change highlight state?
  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);

  // if we are outside and in the start state then return
  if (interactionState == svtkContinuousValueWidgetRepresentation::Outside &&
    self->WidgetState == svtkContinuousValueWidget::Start)
  {
    return;
  }

  // if we are not outside and in the highlighting state then return
  if (interactionState != svtkContinuousValueWidgetRepresentation::Outside &&
    self->WidgetState == svtkContinuousValueWidget::Highlighting)
  {
    return;
  }

  // if we are not outside and in the Start state highlight
  if (interactionState != svtkContinuousValueWidgetRepresentation::Outside &&
    self->WidgetState == svtkContinuousValueWidget::Start)
  {
    self->WidgetRep->Highlight(1);
    self->WidgetState = svtkContinuousValueWidget::Highlighting;
    self->Render();
    return;
  }

  // if we are outside but in the highlight state then stop highlighting
  if (self->WidgetState == svtkContinuousValueWidget::Highlighting &&
    interactionState == svtkContinuousValueWidgetRepresentation::Outside)
  {
    self->WidgetRep->Highlight(0);
    self->WidgetState = svtkContinuousValueWidget::Start;
    self->Render();
    return;
  }

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->WidgetInteraction(eventPos);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-----------------------------------------------------------------
void svtkContinuousValueWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkContinuousValueWidget* self = reinterpret_cast<svtkContinuousValueWidget*>(w);

  if (self->WidgetState != svtkContinuousValueWidget::Adjusting)
  {
    return;
  }

  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);
  if (interactionState == svtkContinuousValueWidgetRepresentation::Outside)
  {
    self->WidgetRep->Highlight(0);
    self->WidgetState = svtkContinuousValueWidget::Start;
  }
  else
  {
    self->WidgetState = svtkContinuousValueWidget::Highlighting;
  }

  // The state returns to unselected
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//-----------------------------------------------------------------
void svtkContinuousValueWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
