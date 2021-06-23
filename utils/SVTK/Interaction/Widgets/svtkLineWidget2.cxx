/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLineWidget2.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkHandleWidget.h"
#include "svtkLineRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkLineWidget2);

//----------------------------------------------------------------------------
svtkLineWidget2::svtkLineWidget2()
{
  this->WidgetState = svtkLineWidget2::Start;
  this->ManagesCursor = 1;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = svtkHandleWidget::New();
  this->Point1Widget->SetPriority(this->Priority - 0.01);
  this->Point1Widget->SetParent(this);
  this->Point1Widget->ManagesCursorOff();

  this->Point2Widget = svtkHandleWidget::New();
  this->Point2Widget->SetPriority(this->Priority - 0.01);
  this->Point2Widget->SetParent(this);
  this->Point2Widget->ManagesCursorOff();

  this->LineHandle = svtkHandleWidget::New();
  this->LineHandle->SetPriority(this->Priority - 0.01);
  this->LineHandle->SetParent(this);
  this->LineHandle->ManagesCursorOff();

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkLineWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkLineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkLineWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkLineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkLineWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkLineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkLineWidget2::MoveAction);

  this->KeyEventCallbackCommand = svtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(svtkLineWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
svtkLineWidget2::~svtkLineWidget2()
{
  this->Point1Widget->Delete();
  this->Point2Widget->Delete();
  this->LineHandle->Delete();
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------
void svtkLineWidget2::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    // Don't actually turn these on until cursor is near the end points or the line.
    this->CreateDefaultRepresentation();
    this->Point1Widget->SetRepresentation(
      reinterpret_cast<svtkLineRepresentation*>(this->WidgetRep)->GetPoint1Representation());
    this->Point1Widget->SetInteractor(this->Interactor);
    this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

    this->Point2Widget->SetRepresentation(
      reinterpret_cast<svtkLineRepresentation*>(this->WidgetRep)->GetPoint2Representation());
    this->Point2Widget->SetInteractor(this->Interactor);
    this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

    this->LineHandle->SetRepresentation(
      reinterpret_cast<svtkLineRepresentation*>(this->WidgetRep)->GetLineHandleRepresentation());
    this->LineHandle->SetInteractor(this->Interactor);
    this->LineHandle->GetRepresentation()->SetRenderer(this->CurrentRenderer);

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
    this->Point1Widget->SetEnabled(0);
    this->Point2Widget->SetEnabled(0);
    this->LineHandle->SetEnabled(0);

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

//----------------------------------------------------------------------
void svtkLineWidget2::SelectAction(svtkAbstractWidget* w)
{
  svtkLineWidget2* self = reinterpret_cast<svtkLineWidget2*>(w);
  if (self->WidgetRep->GetInteractionState() == svtkLineRepresentation::Outside)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetState = svtkLineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)->StartWidgetInteraction(e);
  self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr); // for the handles
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void svtkLineWidget2::TranslateAction(svtkAbstractWidget* w)
{
  svtkLineWidget2* self = reinterpret_cast<svtkLineWidget2*>(w);
  if (self->WidgetRep->GetInteractionState() == svtkLineRepresentation::Outside)
  {
    return;
  }

  // Modify the state, we are selected
  int state = self->WidgetRep->GetInteractionState();
  if (state == svtkLineRepresentation::OnP1)
  {
    reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkLineRepresentation::TranslatingP1);
  }
  else if (state == svtkLineRepresentation::OnP2)
  {
    reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkLineRepresentation::TranslatingP2);
  }
  else
  {
    reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(svtkLineRepresentation::OnLine);
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetState = svtkLineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)->StartWidgetInteraction(eventPos);
  self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr); // for the handles
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkLineWidget2::ScaleAction(svtkAbstractWidget* w)
{
  svtkLineWidget2* self = reinterpret_cast<svtkLineWidget2*>(w);
  if (self->WidgetRep->GetInteractionState() == svtkLineRepresentation::Outside)
  {
    return;
  }

  reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkLineRepresentation::Scaling);
  self->Interactor->Disable();
  self->LineHandle->SetEnabled(0);
  self->Interactor->Enable();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetState = svtkLineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)->StartWidgetInteraction(eventPos);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkLineWidget2::MoveAction(svtkAbstractWidget* w)
{
  svtkLineWidget2* self = reinterpret_cast<svtkLineWidget2*>(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if (self->WidgetState == svtkLineWidget2::Start)
  {
    self->Interactor->Disable(); // avoid extra renders
    self->Point1Widget->SetEnabled(0);
    self->Point2Widget->SetEnabled(0);
    self->LineHandle->SetEnabled(0);

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == svtkLineRepresentation::Outside)
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else // must be near something
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_HAND);
      if (state == svtkLineRepresentation::OnP1)
      {
        self->Point1Widget->SetEnabled(1);
      }
      else if (state == svtkLineRepresentation::OnP2)
      {
        self->Point2Widget->SetEnabled(1);
      }
      else // if ( state == svtkLineRepresentation::OnLine )
      {
        self->LineHandle->SetEnabled(1);
        changed = 1; // movement along the line always needs render
      }
    }
    self->Interactor->Enable(); // avoid extra renders
    if (changed || oldState != state)
    {
      self->Render();
    }
  }
  else // if ( self->WidgetState == svtkLineWidget2::Active )
  {
    // moving something
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr); // handles observe this
    reinterpret_cast<svtkLineRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//----------------------------------------------------------------------
void svtkLineWidget2::EndSelectAction(svtkAbstractWidget* w)
{
  svtkLineWidget2* self = reinterpret_cast<svtkLineWidget2*>(w);
  if (self->WidgetState == svtkLineWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = svtkLineWidget2::Start;
  self->ReleaseFocus();
  self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr); // handles observe this
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Superclass::EndInteraction();
  self->Render();
}

//----------------------------------------------------------------------
void svtkLineWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkLineRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkLineWidget2::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->Point1Widget->SetProcessEvents(pe);
  this->Point2Widget->SetProcessEvents(pe);
  this->LineHandle->SetProcessEvents(pe);
}

//----------------------------------------------------------------------------
void svtkLineWidget2::ProcessKeyEvents(svtkObject*, unsigned long event, void* clientdata, void*)
{
  svtkLineWidget2* self = static_cast<svtkLineWidget2*>(clientdata);
  svtkRenderWindowInteractor* iren = self->GetInteractor();
  svtkLineRepresentation* rep = svtkLineRepresentation::SafeDownCast(self->WidgetRep);
  switch (event)
  {
    case svtkCommand::KeyPressEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
          rep->GetPoint1Representation()->SetXTranslationAxisOn();
          rep->GetPoint2Representation()->SetXTranslationAxisOn();
          rep->GetLineHandleRepresentation()->SetXTranslationAxisOn();
          break;
        case 'y':
        case 'Y':
          rep->GetPoint1Representation()->SetYTranslationAxisOn();
          rep->GetPoint2Representation()->SetYTranslationAxisOn();
          rep->GetLineHandleRepresentation()->SetYTranslationAxisOn();
          break;
        case 'z':
        case 'Z':
          rep->GetPoint1Representation()->SetZTranslationAxisOn();
          rep->GetPoint2Representation()->SetZTranslationAxisOn();
          rep->GetLineHandleRepresentation()->SetZTranslationAxisOn();
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
          rep->GetPoint1Representation()->SetTranslationAxisOff();
          rep->GetPoint2Representation()->SetTranslationAxisOff();
          rep->GetLineHandleRepresentation()->SetTranslationAxisOff();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkLineWidget2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
