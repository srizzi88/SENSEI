/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPlaneWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImplicitPlaneWidget2.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkEventData.h"
#include "svtkImplicitPlaneRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStdString.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkImplicitPlaneWidget2);

// The implicit plane widget observes its representation. The representation
// may invoke an InteractionEvent when the camera moves when LockedNormalToCamera
// is enabled.
class svtkInteractionCallback : public svtkCommand
{
public:
  static svtkInteractionCallback* New() { return new svtkInteractionCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::ModifiedEvent:
        this->ImplicitPlaneWidget->InvokeInteractionCallback();
        break;
    }
  }
  svtkImplicitPlaneWidget2* ImplicitPlaneWidget;
};

//----------------------------------------------------------------------------
svtkImplicitPlaneWidget2::svtkImplicitPlaneWidget2()
{
  this->WidgetState = svtkImplicitPlaneWidget2::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkImplicitPlaneWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkImplicitPlaneWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale,
    this, svtkImplicitPlaneWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkImplicitPlaneWidget2::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 30, 1,
    "Up", svtkWidgetEvent::Up, this, svtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 28, 1,
    "Right", svtkWidgetEvent::Up, this, svtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 31, 1,
    "Down", svtkWidgetEvent::Down, this, svtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 29, 1,
    "Left", svtkWidgetEvent::Down, this, svtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'x', 1,
    "x", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'X', 1,
    "X", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'y', 1,
    "y", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'Y', 1,
    "Y", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'z', 1,
    "z", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::AnyModifier, 'Z', 1,
    "Z", svtkWidgetEvent::ModifyEvent, this, svtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'x',
    1, "x", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'X',
    1, "X", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'y',
    1, "y", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'Y',
    1, "Y", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'z',
    1, "z", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkEvent::AnyModifier, 'Z',
    1, "Z", svtkWidgetEvent::Reset, this, svtkImplicitPlaneWidget2::TranslationAxisUnLock);

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select3D,
      this, svtkImplicitPlaneWidget2::SelectAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::EndSelect3D, this, svtkImplicitPlaneWidget2::EndSelectAction3D);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D,
      this, svtkImplicitPlaneWidget2::MoveAction3D);
  }

  this->InteractionCallback = svtkInteractionCallback::New();
  this->InteractionCallback->ImplicitPlaneWidget = this;
}

//----------------------------------------------------------------------------
svtkImplicitPlaneWidget2::~svtkImplicitPlaneWidget2()
{
  this->InteractionCallback->Delete();
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::SelectAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkImplicitPlaneWidget2::SelectAction3D(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = svtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::TranslateAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::ScaleAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitPlaneRepresentation::Scaling);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::MoveAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor && self->WidgetState != svtkImplicitPlaneWidget2::Active)
  {
    int oldInteractionState =
      reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)->GetInteractionState();

    reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkImplicitPlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    changed = self->UpdateCursorShape(state);
    reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
      ->SetInteractionState(oldInteractionState);
    changed = (changed || state != oldInteractionState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == svtkImplicitPlaneWidget2::Start)
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
void svtkImplicitPlaneWidget2::MoveAction3D(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == svtkImplicitPlaneWidget2::Start)
  {
    return;
  }

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::EndSelectAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  if (self->WidgetState != svtkImplicitPlaneWidget2::Active ||
    self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = svtkImplicitPlaneWidget2::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::EndSelectAction3D(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  if (self->WidgetState != svtkImplicitPlaneWidget2::Active ||
    self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = svtkImplicitPlaneWidget2::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::MovePlaneAction(svtkAbstractWidget* w)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(w);

  reinterpret_cast<svtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkImplicitPlaneRepresentation::Moving);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->ComputeInteractionState(X, Y);

  if (self->WidgetRep->GetInteractionState() == svtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Invoke all of the events associated with moving the plane
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);

  // Move the plane
  double factor = (self->Interactor->GetControlKey() ? 0.5 : 1.0);
  if (svtkStdString(self->Interactor->GetKeySym()) == svtkStdString("Down") ||
    svtkStdString(self->Interactor->GetKeySym()) == svtkStdString("Left"))
  {
    self->GetImplicitPlaneRepresentation()->BumpPlane(-1, factor);
  }
  else
  {
    self->GetImplicitPlaneRepresentation()->BumpPlane(1, factor);
  }
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::SetEnabled(int enabling)
{
  if (this->Enabled == enabling)
  {
    return;
  }

  if (this->GetCurrentRenderer() && !enabling)
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }

  Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkImplicitPlaneRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkImplicitPlaneWidget2::SetRepresentation(svtkImplicitPlaneRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
int svtkImplicitPlaneWidget2::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == svtkImplicitPlaneRepresentation::Outside)
    {
      return this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else if (state == svtkImplicitPlaneRepresentation::MovingOutline)
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
void svtkImplicitPlaneWidget2::SetLockNormalToCamera(int lock)
{
  if (!this->GetImplicitPlaneRepresentation() || !this->Enabled || !this->GetCurrentRenderer())
  {
    return;
  }

  this->GetImplicitPlaneRepresentation()->SetLockNormalToCamera(lock);

  // We assume that the renderer of the Widget cannot be changed without
  // previously being disabled.
  if (lock)
  {
    // We Observe the Camera && make the update
    this->GetCurrentRenderer()->GetActiveCamera()->AddObserver(
      svtkCommand::ModifiedEvent, this->InteractionCallback, this->Priority);

    this->GetImplicitPlaneRepresentation()->SetNormalToCamera();
    this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }
  else
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }
}

//----------------------------------------------------------------------------
void svtkImplicitPlaneWidget2::InvokeInteractionCallback()
{
  svtkMTimeType previousMtime;
  svtkImplicitPlaneRepresentation* widgetRep =
    reinterpret_cast<svtkImplicitPlaneRepresentation*>(this->WidgetRep);

  if (widgetRep->GetLockNormalToCamera())
  {
    previousMtime = widgetRep->GetMTime();
    this->GetImplicitPlaneRepresentation()->SetNormalToCamera();

    if (widgetRep->GetMTime() > previousMtime)
    {
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    }
  }
}

//----------------------------------------------------------------------------
void svtkImplicitPlaneWidget2::TranslationAxisLock(svtkAbstractWidget* widget)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(widget);
  svtkImplicitPlaneRepresentation* rep =
    svtkImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep);
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
void svtkImplicitPlaneWidget2::TranslationAxisUnLock(svtkAbstractWidget* widget)
{
  svtkImplicitPlaneWidget2* self = reinterpret_cast<svtkImplicitPlaneWidget2*>(widget);
  svtkImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep)->SetTranslationAxisOff();
}

//----------------------------------------------------------------------------
void svtkImplicitPlaneWidget2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
