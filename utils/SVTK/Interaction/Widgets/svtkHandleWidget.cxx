/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHandleWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkEventData.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkHandleWidget);

//----------------------------------------------------------------------------------
svtkHandleWidget::svtkHandleWidget()
{
  // Set the initial state
  this->WidgetState = svtkHandleWidget::Inactive;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkHandleWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkHandleWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkHandleWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkHandleWidget::MoveAction);

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select3D,
      this, svtkHandleWidget::SelectAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::EndSelect3D, this, svtkHandleWidget::EndSelectAction);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D, this, svtkHandleWidget::MoveAction3D);
  }

  this->ShowInactive = false;
  this->EnableAxisConstraint = 1;
  this->EnableTranslation = 1;
  this->AllowHandleResize = 1;

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkHandleWidget::ProcessKeyEvents);
}

//----------------------------------------------------------------------------------
svtkHandleWidget::~svtkHandleWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------
void svtkHandleWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkPointHandleRepresentation3D::New();
  }
}

//-------------------------------------------------------------------------
void svtkHandleWidget::SetCursor(int cState)
{
  if (this->ManagesCursor)
  {
    switch (cState)
    {
      case svtkHandleRepresentation::Outside:
        this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
        break;
      default:
        this->RequestCursorShape(SVTK_CURSOR_HAND);
    }
  }
}

//-------------------------------------------------------------------------
void svtkHandleWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->WidgetRep->ComputeInteractionState(X, Y);
  if (self->WidgetRep->GetInteractionState() == svtkHandleRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->WidgetState = svtkHandleWidget::Active;
  reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkHandleRepresentation::Selecting);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void svtkHandleWidget::SelectAction3D(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (self->WidgetRep->GetInteractionState() == svtkHandleRepresentation::Outside)
  {
    return;
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = svtkHandleWidget::Active;
  reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkHandleRepresentation::Selecting);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void svtkHandleWidget::TranslateAction(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  double eventPos[2];
  eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  if (self->WidgetRep->GetInteractionState() == svtkHandleRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkHandleWidget::Active;
  reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkHandleRepresentation::Translating);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void svtkHandleWidget::ScaleAction(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  if (self->AllowHandleResize)
  {

    double eventPos[2];
    eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
    eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);

    self->WidgetRep->StartWidgetInteraction(eventPos);
    if (self->WidgetRep->GetInteractionState() == svtkHandleRepresentation::Outside)
    {
      return;
    }

    // We are definitely selected
    self->WidgetState = svtkHandleWidget::Active;
    reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkHandleRepresentation::Scaling);

    self->GenericAction(self);
  }
}

//-------------------------------------------------------------------------
void svtkHandleWidget::GenericAction(svtkHandleWidget* self)
{
  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Check to see whether motion is constrained
  if (self->Interactor->GetShiftKey() && self->EnableAxisConstraint)
  {
    reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOn();
  }
  else
  {
    reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOff();
  }

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkHandleWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  if (self->WidgetState != svtkHandleWidget::Active)
  {
    return;
  }

  // Return state to not selected
  self->WidgetState = svtkHandleWidget::Start;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->WidgetState = svtkHandleWidget::Start;
  self->Render();
}

//-------------------------------------------------------------------------
void svtkHandleWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState == svtkHandleWidget::Start)
  {
    int state = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeInteractionState(X, Y);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    // Must rerender if we change appearance
    if (reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)->GetActiveRepresentation() &&
      state != self->WidgetRep->GetInteractionState())
    {
      self->Render();
    }
    return;
  }

  if (!self->EnableTranslation)
  {
    return;
  }

  // Okay, adjust the representation
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkHandleWidget::MoveAction3D(svtkAbstractWidget* w)
{
  svtkHandleWidget* self = reinterpret_cast<svtkHandleWidget*>(w);

  // Set the cursor appropriately
  if (self->WidgetState == svtkHandleWidget::Start)
  {
    int state = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeComplexInteractionState(
      self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);

    self->SetCursor(self->WidgetRep->GetInteractionState());

    // Must rerender if we change appearance
    if (reinterpret_cast<svtkHandleRepresentation*>(self->WidgetRep)->GetActiveRepresentation() &&
      state != self->WidgetRep->GetInteractionState())
    {
      self->Render();
    }
    return;
  }

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkHandleWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Allow Handle Resize: " << (this->AllowHandleResize ? "On\n" : "Off\n");

  os << indent << "Enable Axis Constraint: " << (this->EnableAxisConstraint ? "On\n" : "Off\n");

  os << indent << "Show Inactive: " << (this->ShowInactive ? "On\n" : "Off\n");
  os << indent << "WidgetState: " << this->WidgetState << endl;
}

//-------------------------------------------------------------------------
void svtkHandleWidget::SetEnabled(int enabling)
{
  int enabled = this->Enabled;
  if (this->Enabled == enabling)
  {
    return;
  }
  if (!this->ShowInactive)
  {
    // Forward to superclass
    this->Superclass::SetEnabled(enabling);
    if (enabling)
    {
      this->WidgetState = svtkHandleWidget::Start;
    }
    else
    {
      this->WidgetState = svtkHandleWidget::Inactive;
    }
  }
  else
  {
    if (enabling) //----------------
    {
      this->Superclass::SetEnabled(enabling);
      this->WidgetState = svtkHandleWidget::Start;
    }

    else // disabling------------------
    {
      svtkDebugMacro(<< "Disabling widget");

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

      this->WidgetState = svtkHandleWidget::Inactive;
      this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    }
  }

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

//----------------------------------------------------------------------------
void svtkHandleWidget::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkHandleWidget* self = static_cast<svtkHandleWidget*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkHandleRepresentation* rep = svtkHandleRepresentation::SafeDownCast(self->WidgetRep);
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
