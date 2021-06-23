/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitCylinderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImplicitCylinderWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkImplicitCylinderRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkImplicitCylinderWidget);

//----------------------------------------------------------------------------
svtkImplicitCylinderWidget::svtkImplicitCylinderWidget()
{
  this->WidgetState = svtkImplicitCylinderWidget::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkImplicitCylinderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkImplicitCylinderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkImplicitCylinderWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkImplicitCylinderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale,
    this, svtkImplicitCylinderWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkImplicitCylinderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkImplicitCylinderWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 30, 1,
    "Up", svtkWidgetEvent::Up, this, svtkImplicitCylinderWidget::MoveCylinderAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 28, 1,
    "Right", svtkWidgetEvent::Up, this, svtkImplicitCylinderWidget::MoveCylinderAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 31, 1,
    "Down", svtkWidgetEvent::Down, this, svtkImplicitCylinderWidget::MoveCylinderAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 29, 1,
    "Left", svtkWidgetEvent::Down, this, svtkImplicitCylinderWidget::MoveCylinderAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'x', 1,
    "x", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'X', 1,
    "X", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'y', 1,
    "y", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'Y', 1,
    "Y", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'z', 1,
    "z", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'Z', 1,
    "Z", svtkWidgetEvent::ModifyEvent, this, svtkImplicitCylinderWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'x',
    1, "x", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'X',
    1, "X", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'y',
    1, "y", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'Y',
    1, "Y", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'z',
    1, "z", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'Z',
    1, "Z", svtkWidgetEvent::Reset, this, svtkImplicitCylinderWidget::TranslationAxisUnLock);
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to update the radius, axis and center as appropriate
  reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitCylinderRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitCylinderRepresentation::Outside)
  {
    return;
  }

  if (self->Interactor->GetControlKey() &&
    interactionState == svtkImplicitCylinderRepresentation::MovingCenter)
  {
    reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkImplicitCylinderRepresentation::TranslatingCenter);
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitCylinderWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::TranslateAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitCylinderRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitCylinderRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitCylinderWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::ScaleAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitCylinderRepresentation::Scaling);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitCylinderRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitCylinderWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking simple geometry
  // like the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor && self->WidgetState != svtkImplicitCylinderWidget::Active)
  {
    int oldInteractionState =
      reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)->GetInteractionState();

    reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkImplicitCylinderRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    changed = self->UpdateCursorShape(state);
    reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
      ->SetInteractionState(oldInteractionState);
    changed = (changed || state != oldInteractionState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == svtkImplicitCylinderWidget::Start)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

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
void svtkImplicitCylinderWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  if (self->WidgetState != svtkImplicitCylinderWidget::Active ||
    self->WidgetRep->GetInteractionState() == svtkImplicitCylinderRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = svtkImplicitCylinderWidget::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
                            ->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::MoveCylinderAction(svtkAbstractWidget* w)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(w);

  reinterpret_cast<svtkImplicitCylinderRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitCylinderRepresentation::Moving);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->ComputeInteractionState(X, Y);

  // The cursor must be over part of the widget for these key presses to work
  if (self->WidgetRep->GetInteractionState() == svtkImplicitCylinderRepresentation::Outside)
  {
    return;
  }

  // Invoke all of the events associated with moving the cylinder
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);

  // Move the cylinder
  double factor = (self->Interactor->GetControlKey() ? 0.5 : 1.0);
  if (svtkStdString(self->Interactor->GetKeySym()) == svtkStdString("Down") ||
    svtkStdString(self->Interactor->GetKeySym()) == svtkStdString("Left"))
  {
    self->GetCylinderRepresentation()->BumpCylinder(-1, factor);
  }
  else
  {
    self->GetCylinderRepresentation()->BumpCylinder(1, factor);
  }
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkImplicitCylinderRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkImplicitCylinderWidget::SetRepresentation(svtkImplicitCylinderRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
int svtkImplicitCylinderWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == svtkImplicitCylinderRepresentation::Outside)
    {
      return this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else if (state == svtkImplicitCylinderRepresentation::MovingOutline)
    {
      return this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
    }
    else
    {
      return this->RequestCursorShape(SVTK_CURSOR_HAND);
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkImplicitCylinderWidget::TranslationAxisLock(svtkAbstractWidget* widget)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(widget);
  svtkImplicitCylinderRepresentation* rep =
    svtkImplicitCylinderRepresentation::SafeDownCast(self->WidgetRep);
  if (self->Interactor->GetKeyCode() == 'x' || self->Interactor->GetKeyCode() == 'X')
  {
    rep->SetXTranslationAxisOn();
  }
  if (self->Interactor->GetKeyCode() == 'y' || self->Interactor->GetKeyCode() == 'Y')
  {
    rep->SetYTranslationAxisOn();
  }
  if (self->Interactor->GetKeyCode() == 'z' || self->Interactor->GetKeyCode() == 'Z')
  {
    rep->SetZTranslationAxisOn();
  }
}

//----------------------------------------------------------------------------
void svtkImplicitCylinderWidget::TranslationAxisUnLock(svtkAbstractWidget* widget)
{
  svtkImplicitCylinderWidget* self = reinterpret_cast<svtkImplicitCylinderWidget*>(widget);
  svtkImplicitCylinderRepresentation::SafeDownCast(self->WidgetRep)->SetTranslationAxisOff();
}

//----------------------------------------------------------------------------
void svtkImplicitCylinderWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
