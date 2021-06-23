/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHoverWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHoverWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkHoverWidget);

//-------------------------------------------------------------------------
svtkHoverWidget::svtkHoverWidget()
{
  this->WidgetState = Start;
  this->TimerDuration = 250;

  // Okay, define the events for this widget. Note that we look for extra events
  // (like button press) because without it the hover widget thinks nothing has changed
  // and doesn't begin retiming.
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MiddleButtonPressEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseWheelForwardEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseWheelBackwardEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::TimerEvent, svtkWidgetEvent::TimedOut, this, svtkHoverWidget::HoverAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 13, 1,
    "Return", svtkWidgetEvent::Select, this, svtkHoverWidget::SelectAction);
}

//-------------------------------------------------------------------------
svtkHoverWidget::~svtkHoverWidget() = default;

//----------------------------------------------------------------------
void svtkHoverWidget::SetEnabled(int enabling)
{
  if (enabling) //----------------
  {
    svtkDebugMacro(<< "Enabling widget");

    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    if (!this->Interactor)
    {
      svtkErrorMacro(<< "The interactor must be set prior to enabling the widget");
      return;
    }

    // We're ready to enable
    this->Enabled = 1;

    // listen for the events found in the EventTranslator
    this->EventTranslator->AddEventsToInteractor(
      this->Interactor, this->EventCallbackCommand, this->Priority);

    // Start off the timer
    this->TimerId = this->Interactor->CreateRepeatingTimer(this->TimerDuration);
    this->WidgetState = svtkHoverWidget::Timing;

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling------------------
  {
    svtkDebugMacro(<< "Disabling widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
  }
}

//-------------------------------------------------------------------------
void svtkHoverWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkHoverWidget* self = reinterpret_cast<svtkHoverWidget*>(w);
  if (self->WidgetState == svtkHoverWidget::Timing)
  {
    self->Interactor->DestroyTimer(self->TimerId);
  }
  else // we have already timed out, on this move we begin retiming
  {
    self->WidgetState = svtkHoverWidget::Timing;
    self->SubclassEndHoverAction();
    self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  }
  self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
}

//-------------------------------------------------------------------------
void svtkHoverWidget::HoverAction(svtkAbstractWidget* w)
{
  svtkHoverWidget* self = reinterpret_cast<svtkHoverWidget*>(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if (timerId == self->TimerId && self->WidgetState == svtkHoverWidget::Timing)
  {
    self->Interactor->DestroyTimer(self->TimerId);
    self->WidgetState = svtkHoverWidget::TimedOut;
    self->SubclassHoverAction();
    self->InvokeEvent(svtkCommand::TimerEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1); // no one else gets this timer
  }
}

//-------------------------------------------------------------------------
void svtkHoverWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkHoverWidget* self = reinterpret_cast<svtkHoverWidget*>(w);

  // If widget is hovering we grab the selection event
  if (self->WidgetState == svtkHoverWidget::TimedOut)
  {
    self->SubclassSelectAction();
    self->InvokeEvent(svtkCommand::WidgetActivateEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1); // no one else gets this event
  }
}

//-------------------------------------------------------------------------
void svtkHoverWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Timer Duration: " << this->TimerDuration << "\n";
}
