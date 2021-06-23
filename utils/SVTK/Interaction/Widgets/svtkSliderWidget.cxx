/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSliderWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliderRepresentation3D.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkSliderWidget);

//----------------------------------------------------------------------------------
svtkSliderWidget::svtkSliderWidget()
{
  // Set the initial state
  this->WidgetState = svtkSliderWidget::Start;

  // Assign initial values
  this->AnimationMode = svtkSliderWidget::Jump;
  this->NumberOfAnimationSteps = 24;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkSliderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkSliderWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkSliderWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void svtkSliderWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkSliderRepresentation3D::New();
  }
}

//----------------------------------------------------------------------------------
void svtkSliderWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkSliderWidget* self = reinterpret_cast<svtkSliderWidget*>(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(
      static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1])))
  {
    self->WidgetState = svtkSliderWidget::Start;
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
  self->GrabFocus(self->EventCallbackCommand);
  if (interactionState == svtkSliderRepresentation::Slider)
  {
    self->WidgetState = svtkSliderWidget::Sliding;
  }
  else
  {
    self->WidgetState = svtkSliderWidget::Animating;
  }

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkSliderWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkSliderWidget* self = reinterpret_cast<svtkSliderWidget*>(w);

  // See whether we're active
  if (self->WidgetState == svtkSliderWidget::Start ||
    self->WidgetState == svtkSliderWidget::Animating)
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
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkSliderWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkSliderWidget* self = reinterpret_cast<svtkSliderWidget*>(w);

  if (self->WidgetState == svtkSliderWidget::Start)
  {
    return;
  }

  // If animating we do it and return
  if (self->WidgetState == svtkSliderWidget::Animating)
  {
    self->AnimateSlider(self->WidgetRep->GetInteractionState());
  }

  // Highlight if necessary
  self->WidgetRep->Highlight(0);

  // The state returns to unselected
  self->WidgetState = svtkSliderWidget::Start;
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkSliderWidget::AnimateSlider(int selectionState)
{
  // Get the representation and grab some information
  svtkSliderRepresentation* sliderRep = reinterpret_cast<svtkSliderRepresentation*>(this->WidgetRep);

  // If the slider bead has been selected, then nothing happens
  if (selectionState == svtkSliderRepresentation::Outside ||
    selectionState == svtkSliderRepresentation::Slider)
  {
    return;
  }

  // Depending on animation mode, we'll jump to the pick point or
  // animate towards it.
  double minValue = sliderRep->GetMinimumValue();
  double maxValue = sliderRep->GetMaximumValue();
  double pickedT = sliderRep->GetPickedT();

  if (this->AnimationMode == svtkSliderWidget::Jump)
  {
    if (selectionState == svtkSliderRepresentation::Tube)
    {
      sliderRep->SetValue(minValue + pickedT * (maxValue - minValue));
    }

    else if (selectionState == svtkSliderRepresentation::LeftCap)
    {
      sliderRep->SetValue(minValue);
    }

    else if (selectionState == svtkSliderRepresentation::RightCap)
    {
      sliderRep->SetValue(maxValue);
    }
    sliderRep->BuildRepresentation();
    this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (this->AnimationMode == svtkSliderWidget::Animate)
  {
    double targetValue = minValue, originalValue = sliderRep->GetValue();
    if (selectionState == svtkSliderRepresentation::Tube)
    {
      targetValue = minValue + pickedT * (maxValue - minValue);
    }

    else if (selectionState == svtkSliderRepresentation::LeftCap)
    {
      targetValue = minValue;
    }

    else if (selectionState == svtkSliderRepresentation::RightCap)
    {
      targetValue = maxValue;
    }

    // Okay, animate the slider
    double value;
    for (int i = 0; i < this->NumberOfAnimationSteps; i++)
    {
      value = originalValue +
        (static_cast<double>(i + 1) / this->NumberOfAnimationSteps) * (targetValue - originalValue);
      sliderRep->SetValue(value);
      sliderRep->BuildRepresentation();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      this->Render();
    }
  }

  this->WidgetState = svtkSliderWidget::Start;
}

//----------------------------------------------------------------------------------
void svtkSliderWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Animation Mode: ";
  switch (this->AnimationMode)
  {
    case svtkSliderWidget::Jump:
      os << "Jump\n";
      break;
    case svtkSliderWidget::Animate:
      os << "Animate\n";
      break;
    default:
      os << "AnimateOff\n";
  }

  os << indent << "Number of Animation Steps: " << this->NumberOfAnimationSteps << "\n";
}
