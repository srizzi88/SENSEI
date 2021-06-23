/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelopipedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkParallelopipedWidget.h"
#include "svtkActor.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkGarbageCollector.h"
#include "svtkHandleWidget.h"
#include "svtkInteractorObserver.h"
#include "svtkObjectFactory.h"
#include "svtkParallelopipedRepresentation.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"
#include "svtkWidgetSet.h"

svtkStandardNewMacro(svtkParallelopipedWidget);

//----------------------------------------------------------------------
svtkParallelopipedWidget::svtkParallelopipedWidget()
{
  // Allow chairs to be created.
  this->EnableChairCreation = 1;

  // 8 handles for the 8 corners of the piped.
  this->HandleWidgets = new svtkHandleWidget*[8];
  for (int i = 0; i < 8; i++)
  {
    this->HandleWidgets[i] = svtkHandleWidget::New();

    // The widget gets a higher priority than the handles.
    this->HandleWidgets[i]->SetPriority(this->Priority - 0.01);
    this->HandleWidgets[i]->SetParent(this);

    // The piped widget will decide what cursor to show.
    this->HandleWidgets[i]->ManagesCursorOff();
  }

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkEvent::NoModifier, 0,
    1, nullptr, svtkParallelopipedWidget::RequestResizeEvent, this,
    svtkParallelopipedWidget::RequestResizeAlongAnAxisCallback);
  // Commented out by Will because it is unstable code
  //            this, svtkParallelopipedWidget::RequestResizeCallback);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkEvent::ShiftModifier,
    0, 1, nullptr, svtkParallelopipedWidget::RequestResizeAlongAnAxisEvent, this,
    svtkParallelopipedWidget::RequestResizeAlongAnAxisCallback);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkEvent::ControlModifier, 0, 1, nullptr, svtkParallelopipedWidget::RequestChairModeEvent, this,
    svtkParallelopipedWidget::RequestChairModeCallback);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkParallelopipedWidget::OnLeftButtonUpCallback);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this,
    svtkParallelopipedWidget::OnMouseMoveCallback);

  this->WidgetSet = nullptr;
}

//----------------------------------------------------------------------
svtkParallelopipedWidget::~svtkParallelopipedWidget()
{
  for (int i = 0; i < 8; i++)
  {
    this->HandleWidgets[i]->Delete();
  }
  delete[] this->HandleWidgets;
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkParallelopipedRepresentation::New();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::SetEnabled(int enabling)
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

    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];

    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X, Y));

      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    // We're ready to enable
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // listen for the events found in the EventTranslator
    if (!this->Parent)
    {
      this->EventTranslator->AddEventsToInteractor(
        this->Interactor, this->EventCallbackCommand, this->Priority);
    }
    else
    {
      this->EventTranslator->AddEventsToParent(
        this->Parent, this->EventCallbackCommand, this->Priority);
    }

    // Enable each of the handle widgets.
    for (int i = 0; i < 8; i++)
    {
      if (this->HandleWidgets[i])
      {
        this->HandleWidgets[i]->SetRepresentation(
          svtkParallelopipedRepresentation::SafeDownCast(this->WidgetRep)
            ->GetHandleRepresentation(i));
        this->HandleWidgets[i]->SetInteractor(this->Interactor);
        this->HandleWidgets[i]->GetRepresentation()->SetRenderer(this->CurrentRenderer);

        this->HandleWidgets[i]->SetEnabled(enabling);
      }
    }

    if (this->ManagesCursor)
    {
      this->WidgetRep->ComputeInteractionState(X, Y);
      this->SetCursor(this->WidgetRep->GetInteractionState());
    }

    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);

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

    // don't listen for events any more
    if (!this->Parent)
    {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
    else
    {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
    }

    // Disable each of the handle widgets.
    for (int i = 0; i < 8; i++)
    {
      if (this->HandleWidgets[i])
      {
        this->HandleWidgets[i]->SetEnabled(enabling);
      }
    }

    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
  }

  // Should only render if there is no parent
  if (this->Interactor && !this->Parent)
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::RequestResizeCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey() |
    self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(svtkParallelopipedRepresentation::RequestResizeParallelopiped);

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1], modifier);
  self->SetCursor(interactionState);

  if (interactionState != svtkParallelopipedRepresentation::Outside)
  {
    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->Interactor->Render();
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget ::RequestResizeAlongAnAxisCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey() |
    self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(svtkParallelopipedRepresentation::RequestResizeParallelopipedAlongAnAxis);

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1], modifier);

  self->SetCursor(interactionState);

  if (interactionState == svtkParallelopipedRepresentation::Inside)
  {
    // We did not select any of the handles, nevertheless we are at least
    // inside the parallelopiped. We could do things like Translate etc. So
    // we will delegate responsibility to those callbacks
    self->TranslateCallback(self);
  }

  else if (interactionState != svtkParallelopipedRepresentation::Outside)
  {
    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->Interactor->Render();
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::RequestChairModeCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);

  if (!self->EnableChairCreation)
  {
    return;
  }

  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey() |
    self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(svtkParallelopipedRepresentation::RequestChairMode);

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1], modifier);
  self->SetCursor(interactionState);

  if (interactionState != svtkParallelopipedRepresentation::Outside)
  {
    // Ok, so we did select a handle.... Render..

    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->Interactor->Render();
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::TranslateCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  // We know we are inside the parallelopiped.
  // Change the cursor to the Translate thingie.
  self->SetCursor(svtkParallelopipedRepresentation::TranslatingParallelopiped);
  rep->SetInteractionState(svtkParallelopipedRepresentation::TranslatingParallelopiped);

  // Dispatch to all widgets in the set.
  if (self->WidgetSet)
  {
    self->WidgetSet->DispatchAction(self, &svtkParallelopipedWidget::BeginTranslateAction);
  }
  else
  {
    self->BeginTranslateAction(self);
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget ::BeginTranslateAction(svtkParallelopipedWidget* svtkNotUsed(dispatcher))
{
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(this->WidgetRep);

  // We know we are inside the parallelopiped.
  // Change the cursor to the Translate thingie.
  rep->SetInteractionState(svtkParallelopipedRepresentation::TranslatingParallelopiped);
  this->SetCursor(rep->GetInteractionState());

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget ::TranslateAction(svtkParallelopipedWidget* svtkNotUsed(dispatcher))
{
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(this->WidgetRep);
  rep->Translate(this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::OnLeftButtonUpCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  int interactionState = rep->GetInteractionState();

  // Reset the state
  rep->SetInteractionState(svtkParallelopipedRepresentation::Outside);

  // Let the representation re-compute what the appropriate state is.
  const int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey() |
    self->Interactor->GetAltKey();
  int newInteractionState = rep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1], modifier);

  // If we computed a different interaction state than the one we were in,
  // render in response to any changes.
  if (newInteractionState != interactionState)
  {
    self->Interactor->Render();
    self->SetCursor(newInteractionState);
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  }
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::OnMouseMoveCallback(svtkAbstractWidget* w)
{
  svtkParallelopipedWidget* self = reinterpret_cast<svtkParallelopipedWidget*>(w);
  svtkParallelopipedRepresentation* rep =
    reinterpret_cast<svtkParallelopipedRepresentation*>(self->WidgetRep);

  int interactionState = rep->GetInteractionState();
  int newInteractionState = interactionState;

  if (interactionState == svtkParallelopipedRepresentation ::TranslatingParallelopiped)
  {
    // Dispatch to all widgets in the set.
    if (self->WidgetSet)
    {
      self->WidgetSet->DispatchAction(self, &svtkParallelopipedWidget::TranslateAction);
    }
    else
    {
      self->TranslateAction(self);
    }
  }

  else
  {
    // Let the representation re-compute what the appropriate state is.
    const int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey() |
      self->Interactor->GetAltKey();
    newInteractionState = rep->ComputeInteractionState(
      self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1], modifier);
  }

  // If we computed a different interaction state than the one we were in,
  // render in response to any changes. Also take care of trivial cases that
  // require no rendering.
  if (newInteractionState != interactionState ||
    (newInteractionState != svtkParallelopipedRepresentation::Inside &&
      newInteractionState != svtkParallelopipedRepresentation::Outside))
  {
    self->Interactor->Render();
    self->SetCursor(newInteractionState);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }
}

//-------------------------------------------------------------------------
void svtkParallelopipedWidget::SetCursor(int state)
{
  switch (state)
  {
    case svtkParallelopipedRepresentation::ResizingParallelopiped:
    case svtkParallelopipedRepresentation::ResizingParallelopipedAlongAnAxis:
      this->RequestCursorShape(SVTK_CURSOR_HAND);
      break;
    default:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
}

//----------------------------------------------------------------------------
void svtkParallelopipedWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);
  for (int i = 0; i < 8; i++)
  {
    this->HandleWidgets[i]->SetProcessEvents(pe);
  }
}

//----------------------------------------------------------------------------
void svtkParallelopipedWidget::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->WidgetSet, "WidgetSet");
}

//----------------------------------------------------------------------
void svtkParallelopipedWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Chair Creation: " << (this->EnableChairCreation ? "On\n" : "Off\n");
}
