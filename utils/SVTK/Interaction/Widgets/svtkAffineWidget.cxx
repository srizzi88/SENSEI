/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAffineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAffineWidget.h"
#include "svtkAffineRepresentation2D.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkAffineWidget);

//----------------------------------------------------------------------------------
svtkAffineWidget::svtkAffineWidget()
{
  // Set the initial state
  this->WidgetState = svtkAffineWidget::Start;

  this->ModifierActive = 0;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkAffineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkAffineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkAffineWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkWidgetEvent::ModifyEvent,
    this, svtkAffineWidget::ModifyEventAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyReleaseEvent, svtkWidgetEvent::ModifyEvent,
    this, svtkAffineWidget::ModifyEventAction);
}

//----------------------------------------------------------------------------------
svtkAffineWidget::~svtkAffineWidget() = default;

//----------------------------------------------------------------------
void svtkAffineWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void svtkAffineWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkAffineRepresentation2D::New();
  }
}

//-------------------------------------------------------------------------
void svtkAffineWidget::SetCursor(int cState)
{
  switch (cState)
  {
    case svtkAffineRepresentation::ScaleNE:
    case svtkAffineRepresentation::ScaleSW:
      this->RequestCursorShape(SVTK_CURSOR_SIZESW);
      break;
    case svtkAffineRepresentation::ScaleNW:
    case svtkAffineRepresentation::ScaleSE:
      this->RequestCursorShape(SVTK_CURSOR_SIZENW);
      break;
    case svtkAffineRepresentation::ScaleNEdge:
    case svtkAffineRepresentation::ScaleSEdge:
    case svtkAffineRepresentation::ShearWEdge:
    case svtkAffineRepresentation::ShearEEdge:
      this->RequestCursorShape(SVTK_CURSOR_SIZENS);
      break;
    case svtkAffineRepresentation::ScaleWEdge:
    case svtkAffineRepresentation::ScaleEEdge:
    case svtkAffineRepresentation::ShearNEdge:
    case svtkAffineRepresentation::ShearSEdge:
      this->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      break;
    case svtkAffineRepresentation::Rotate:
      this->RequestCursorShape(SVTK_CURSOR_HAND);
      break;
    case svtkAffineRepresentation::TranslateX:
    case svtkAffineRepresentation::MoveOriginX:
      this->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      break;
    case svtkAffineRepresentation::TranslateY:
    case svtkAffineRepresentation::MoveOriginY:
      this->RequestCursorShape(SVTK_CURSOR_SIZENS);
      break;
    case svtkAffineRepresentation::Translate:
    case svtkAffineRepresentation::MoveOrigin:
      this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      break;
    case svtkAffineRepresentation::Outside:
    default:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
}

//-------------------------------------------------------------------------
void svtkAffineWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkAffineWidget* self = reinterpret_cast<svtkAffineWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey();
  reinterpret_cast<svtkAffineRepresentation*>(self->WidgetRep)
    ->ComputeInteractionState(X, Y, self->ModifierActive);

  if (self->WidgetRep->GetInteractionState() == svtkAffineRepresentation::Outside)
  {
    return;
  }

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = svtkAffineWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkAffineWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkAffineWidget* self = reinterpret_cast<svtkAffineWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState == svtkAffineWidget::Start)
  {
    self->ModifierActive = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey();
    int state = self->WidgetRep->GetInteractionState();
    reinterpret_cast<svtkAffineRepresentation*>(self->WidgetRep)
      ->ComputeInteractionState(X, Y, self->ModifierActive);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    if (state != self->WidgetRep->GetInteractionState())
    {
      self->Render();
    }
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
void svtkAffineWidget::ModifyEventAction(svtkAbstractWidget* w)
{
  svtkAffineWidget* self = reinterpret_cast<svtkAffineWidget*>(w);
  if (self->WidgetState == svtkAffineWidget::Start)
  {
    int modifierActive = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey();
    if (self->ModifierActive != modifierActive)
    {
      self->ModifierActive = modifierActive;
      int X = self->Interactor->GetEventPosition()[0];
      int Y = self->Interactor->GetEventPosition()[1];
      reinterpret_cast<svtkAffineRepresentation*>(self->WidgetRep)
        ->ComputeInteractionState(X, Y, self->ModifierActive);
      self->SetCursor(self->WidgetRep->GetInteractionState());
    }
  }
}

//-------------------------------------------------------------------------
void svtkAffineWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkAffineWidget* self = reinterpret_cast<svtkAffineWidget*>(w);

  if (self->WidgetState != svtkAffineWidget::Active)
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->EndWidgetInteraction(eventPos);

  // return to initial state
  self->WidgetState = svtkAffineWidget::Start;
  self->ModifierActive = 0;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  self->EventCallbackCommand->SetAbortFlag(1);
  self->ReleaseFocus();
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->WidgetState = svtkAffineWidget::Start;
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkAffineWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
