/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSeedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSeedWidget.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkEvent.h"
#include "svtkHandleRepresentation.h"
#include "svtkHandleWidget.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSeedRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include <iterator>
#include <list>

svtkStandardNewMacro(svtkSeedWidget);

// The svtkSeedList is a PIMPLed list<T>.
class svtkSeedList : public std::list<svtkHandleWidget*>
{
};
typedef std::list<svtkHandleWidget*>::iterator svtkSeedListIterator;

//----------------------------------------------------------------------
svtkSeedWidget::svtkSeedWidget()
{
  this->ManagesCursor = 1;
  this->WidgetState = svtkSeedWidget::Start;

  // The widgets for moving the seeds.
  this->Seeds = new svtkSeedList;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkWidgetEvent::AddPoint, this, svtkSeedWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent,
    svtkWidgetEvent::Completed, this, svtkSeedWidget::CompletedAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkSeedWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkSeedWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::NoModifier, 127, 1,
    "Delete", svtkWidgetEvent::Delete, this, svtkSeedWidget::DeleteAction);
  this->Defining = 1;
}

//----------------------------------------------------------------------
void svtkSeedWidget::DeleteSeed(int i)
{
  if (this->Seeds->size() <= static_cast<size_t>(i))
  {
    return;
  }

  svtkSeedRepresentation* rep = static_cast<svtkSeedRepresentation*>(this->WidgetRep);
  if (rep)
  {
    rep->RemoveHandle(i);
  }

  svtkSeedListIterator iter = this->Seeds->begin();
  std::advance(iter, i);
  (*iter)->SetEnabled(0);
  (*iter)->RemoveObservers(svtkCommand::StartInteractionEvent);
  (*iter)->RemoveObservers(svtkCommand::InteractionEvent);
  (*iter)->RemoveObservers(svtkCommand::EndInteractionEvent);
  svtkHandleWidget* w = (*iter);
  this->Seeds->erase(iter);
  w->Delete();
}

//----------------------------------------------------------------------
svtkSeedWidget::~svtkSeedWidget()
{
  // Loop over all seeds releasing their observers and deleting them
  while (!this->Seeds->empty())
  {
    this->DeleteSeed(static_cast<int>(this->Seeds->size()) - 1);
  }
  delete this->Seeds;
}

//----------------------------------------------------------------------
svtkHandleWidget* svtkSeedWidget::GetSeed(int i)
{
  if (this->Seeds->size() <= static_cast<size_t>(i))
  {
    return nullptr;
  }
  svtkSeedListIterator iter = this->Seeds->begin();
  std::advance(iter, i);
  return *iter;
}

//----------------------------------------------------------------------
void svtkSeedWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkSeedRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkSeedWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  svtkSeedListIterator iter;
  for (iter = this->Seeds->begin(); iter != this->Seeds->end(); ++iter)
  {
    (*iter)->SetEnabled(enabling);
  }

  if (!enabling)
  {
    this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    this->WidgetState = svtkSeedWidget::Start;
  }

  this->Render();
}

// The following methods are the callbacks that the seed widget responds to.
//-------------------------------------------------------------------------
void svtkSeedWidget::AddPointAction(svtkAbstractWidget* w)
{
  svtkSeedWidget* self = reinterpret_cast<svtkSeedWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if (self->WidgetState == svtkSeedWidget::MovingSeed)
  {
    return;
  }

  self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // When a seed is placed, a new handle widget must be created and enabled.
  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  if (state == svtkSeedRepresentation::NearSeed)
  {
    self->WidgetState = svtkSeedWidget::MovingSeed;

    // Invoke an event on ourself for the handles
    self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
    self->Superclass::StartInteraction();
    svtkSeedRepresentation* rep = static_cast<svtkSeedRepresentation*>(self->WidgetRep);
    int seedIdx = rep->GetActiveHandle();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, &seedIdx);

    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }

  else if (self->WidgetState != svtkSeedWidget::PlacedSeeds)
  {
    // we are placing a new seed. Just make sure we aren't in a mode which
    // dictates we've placed all seeds.

    self->WidgetState = svtkSeedWidget::PlacingSeeds;
    double e[3];
    e[2] = 0.0;
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);

    svtkSeedRepresentation* rep = reinterpret_cast<svtkSeedRepresentation*>(self->WidgetRep);
    // if the handle representation is constrained, check to see if
    // the position follows the constraint.
    if (!rep->GetHandleRepresentation()->CheckConstraint(self->GetCurrentRenderer(), e))
    {
      return;
    }
    int currentHandleNumber = rep->CreateHandle(e);
    svtkHandleWidget* currentHandle = self->CreateNewHandle();
    rep->SetSeedDisplayPosition(currentHandleNumber, e);
    currentHandle->SetEnabled(1);
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(currentHandleNumber));
    self->InvokeEvent(svtkCommand::InteractionEvent, &(currentHandleNumber));

    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//-------------------------------------------------------------------------
void svtkSeedWidget::CompletedAction(svtkAbstractWidget* w)
{
  svtkSeedWidget* self = reinterpret_cast<svtkSeedWidget*>(w);

  // Do something only if we are in the middle of placing the seeds
  if (self->WidgetState == svtkSeedWidget::PlacingSeeds)
  {
    self->CompleteInteraction();
  }
}

//-------------------------------------------------------------------------
void svtkSeedWidget::CompleteInteraction()
{
  this->WidgetState = svtkSeedWidget::PlacedSeeds;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->Defining = 0;
}

//-------------------------------------------------------------------------
void svtkSeedWidget::RestartInteraction()
{
  this->WidgetState = svtkSeedWidget::Start;
  this->Defining = 1;
}

//-------------------------------------------------------------------------
void svtkSeedWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkSeedWidget* self = reinterpret_cast<svtkSeedWidget*>(w);

  self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);

  // set the cursor shape to a hand if we are near a seed.
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int state = self->WidgetRep->ComputeInteractionState(X, Y);

  // Change the cursor shape to a hand and invoke an interaction event if we
  // are near the seed
  if (state == svtkSeedRepresentation::NearSeed)
  {
    self->RequestCursorShape(SVTK_CURSOR_HAND);

    svtkSeedRepresentation* rep = static_cast<svtkSeedRepresentation*>(self->WidgetRep);
    int seedIdx = rep->GetActiveHandle();
    self->InvokeEvent(svtkCommand::InteractionEvent, &seedIdx);

    self->EventCallbackCommand->SetAbortFlag(1);
  }
  else
  {
    self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }

  self->Render();
}

//-------------------------------------------------------------------------
void svtkSeedWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkSeedWidget* self = reinterpret_cast<svtkSeedWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState != svtkSeedWidget::MovingSeed)
  {
    return;
  }

  // Revert back to the mode we were in prior to selection.
  self->WidgetState = self->Defining ? svtkSeedWidget::PlacingSeeds : svtkSeedWidget::PlacedSeeds;

  // Invoke event for seed handle
  self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Superclass::EndInteraction();
  self->Render();
}

//-------------------------------------------------------------------------
void svtkSeedWidget::DeleteAction(svtkAbstractWidget* w)
{
  svtkSeedWidget* self = reinterpret_cast<svtkSeedWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState != svtkSeedWidget::PlacingSeeds)
  {
    return;
  }

  // Remove last seed
  svtkSeedRepresentation* rep = reinterpret_cast<svtkSeedRepresentation*>(self->WidgetRep);
  int removeId = rep->GetActiveHandle();
  removeId = removeId != -1 ? removeId : static_cast<int>(self->Seeds->size()) - 1;
  // Invoke event for seed handle before actually deleting
  self->InvokeEvent(svtkCommand::DeletePointEvent, &(removeId));

  self->DeleteSeed(removeId);
  // Got this event, abort processing if it
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//----------------------------------------------------------------------
void svtkSeedWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  svtkSeedListIterator iter = this->Seeds->begin();
  for (; iter != this->Seeds->end(); ++iter)
  {
    (*iter)->SetProcessEvents(pe);
  }
}

//----------------------------------------------------------------------
void svtkSeedWidget::SetInteractor(svtkRenderWindowInteractor* rwi)
{
  this->Superclass::SetInteractor(rwi);
  svtkSeedListIterator iter = this->Seeds->begin();
  for (; iter != this->Seeds->end(); ++iter)
  {
    (*iter)->SetInteractor(rwi);
  }
}

//----------------------------------------------------------------------
void svtkSeedWidget::SetCurrentRenderer(svtkRenderer* ren)
{
  this->Superclass::SetCurrentRenderer(ren);
  svtkSeedListIterator iter = this->Seeds->begin();
  for (; iter != this->Seeds->end(); ++iter)
  {
    if (!ren)
    {
      // Disable widget if it's being removed from the renderer
      (*iter)->EnabledOff();
    }
    (*iter)->SetCurrentRenderer(ren);
  }
}

//----------------------------------------------------------------------
// Programmatically create a new handle.
svtkHandleWidget* svtkSeedWidget::CreateNewHandle()
{
  svtkSeedRepresentation* rep = svtkSeedRepresentation::SafeDownCast(this->WidgetRep);
  if (!rep)
  {
    svtkErrorMacro(<< "Please set, or create a default seed representation "
                  << "before adding requesting creation of a new handle.");
    return nullptr;
  }

  // Create the handle widget or reuse an old one
  int currentHandleNumber = static_cast<int>(this->Seeds->size());
  svtkHandleWidget* widget = svtkHandleWidget::New();

  // Configure the handle widget
  widget->SetParent(this);
  widget->SetInteractor(this->Interactor);
  svtkHandleRepresentation* handleRep = rep->GetHandleRepresentation(currentHandleNumber);
  if (!handleRep)
  {
    widget->Delete();
    return nullptr;
  }
  else
  {
    handleRep->SetRenderer(this->CurrentRenderer);
    widget->SetRepresentation(handleRep);

    // Now place the widget into the list of handle widgets (if not already there)
    this->Seeds->push_back(widget);
    return widget;
  }
}

//----------------------------------------------------------------------
void svtkSeedWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
}
