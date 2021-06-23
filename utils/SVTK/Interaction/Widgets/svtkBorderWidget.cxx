/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBorderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBorderWidget.h"
#include "svtkBorderRepresentation.h"
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

svtkStandardNewMacro(svtkBorderWidget);

//-------------------------------------------------------------------------
svtkBorderWidget::svtkBorderWidget()
{
  this->WidgetState = svtkBorderWidget::Start;
  this->Selectable = 1;
  this->Resizable = 1;

  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkBorderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkBorderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkBorderWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkBorderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkBorderWidget::MoveAction);
}

//-------------------------------------------------------------------------
svtkBorderWidget::~svtkBorderWidget() = default;

//-------------------------------------------------------------------------
void svtkBorderWidget::SetCursor(int cState)
{
  if (!this->Resizable && cState != svtkBorderRepresentation::Inside)
  {
    this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    return;
  }

  switch (cState)
  {
    case svtkBorderRepresentation::AdjustingP0:
      this->RequestCursorShape(SVTK_CURSOR_SIZESW);
      break;
    case svtkBorderRepresentation::AdjustingP1:
      this->RequestCursorShape(SVTK_CURSOR_SIZESE);
      break;
    case svtkBorderRepresentation::AdjustingP2:
      this->RequestCursorShape(SVTK_CURSOR_SIZENE);
      break;
    case svtkBorderRepresentation::AdjustingP3:
      this->RequestCursorShape(SVTK_CURSOR_SIZENW);
      break;
    case svtkBorderRepresentation::AdjustingE0:
    case svtkBorderRepresentation::AdjustingE2:
      this->RequestCursorShape(SVTK_CURSOR_SIZENS);
      break;
    case svtkBorderRepresentation::AdjustingE1:
    case svtkBorderRepresentation::AdjustingE3:
      this->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      break;
    case svtkBorderRepresentation::Inside:
      if (reinterpret_cast<svtkBorderRepresentation*>(this->WidgetRep)->GetMoving())
      {
        this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      }
      else
      {
        this->RequestCursorShape(SVTK_CURSOR_HAND);
      }
      break;
    default:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
}

//-------------------------------------------------------------------------
void svtkBorderWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkBorderWidget* self = reinterpret_cast<svtkBorderWidget*>(w);

  if (self->SubclassSelectAction() ||
    self->WidgetRep->GetInteractionState() == svtkBorderRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->WidgetState = svtkBorderWidget::Selected;

  // Picked something inside the widget
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor (i.e., the MoveAction may have set the cursor previously, but this
  // method is necessary to maintain the proper cursor shape)..
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // convert to normalized viewport coordinates
  double XF = static_cast<double>(X);
  double YF = static_cast<double>(Y);
  self->CurrentRenderer->DisplayToNormalizedDisplay(XF, YF);
  self->CurrentRenderer->NormalizedDisplayToViewport(XF, YF);
  self->CurrentRenderer->ViewportToNormalizedViewport(XF, YF);
  double eventPos[2];
  eventPos[0] = XF;
  eventPos[1] = YF;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  if (self->Selectable && self->WidgetRep->GetInteractionState() == svtkBorderRepresentation::Inside)
  {
    svtkBorderRepresentation* rep = reinterpret_cast<svtkBorderRepresentation*>(self->WidgetRep);
    double* fpos1 = rep->GetPositionCoordinate()->GetValue();
    double* fpos2 = rep->GetPosition2Coordinate()->GetValue();

    eventPos[0] = (XF - fpos1[0]) / fpos2[0];
    eventPos[1] = (YF - fpos1[1]) / fpos2[1];

    self->SelectRegion(eventPos);
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkBorderWidget::TranslateAction(svtkAbstractWidget* w)
{
  svtkBorderWidget* self = reinterpret_cast<svtkBorderWidget*>(w);

  if (self->SubclassTranslateAction() ||
    self->WidgetRep->GetInteractionState() == svtkBorderRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->WidgetState = svtkBorderWidget::Selected;
  reinterpret_cast<svtkBorderRepresentation*>(self->WidgetRep)->MovingOn();

  // Picked something inside the widget
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // convert to normalized viewport coordinates
  double XF = static_cast<double>(X);
  double YF = static_cast<double>(Y);
  self->CurrentRenderer->DisplayToNormalizedDisplay(XF, YF);
  self->CurrentRenderer->NormalizedDisplayToViewport(XF, YF);
  self->CurrentRenderer->ViewportToNormalizedViewport(XF, YF);
  double eventPos[2];
  eventPos[0] = XF;
  eventPos[1] = YF;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkBorderWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkBorderWidget* self = reinterpret_cast<svtkBorderWidget*>(w);

  if (self->SubclassMoveAction())
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState == svtkBorderWidget::Start)
  {
    int stateBefore = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeInteractionState(X, Y);
    int stateAfter = self->WidgetRep->GetInteractionState();
    self->SetCursor(stateAfter);

    svtkBorderRepresentation* borderRepresentation =
      reinterpret_cast<svtkBorderRepresentation*>(self->WidgetRep);
    if (self->Selectable || stateAfter != svtkBorderRepresentation::Inside)
    {
      borderRepresentation->MovingOff();
    }
    else
    {
      borderRepresentation->MovingOn();
    }

    if ((borderRepresentation->GetShowVerticalBorder() == svtkBorderRepresentation::BORDER_ACTIVE ||
          borderRepresentation->GetShowHorizontalBorder() ==
            svtkBorderRepresentation::BORDER_ACTIVE) &&
      stateBefore != stateAfter &&
      (stateBefore == svtkBorderRepresentation::Outside ||
        stateAfter == svtkBorderRepresentation::Outside))
    {
      self->Render();
    }
    return;
  }

  if (!self->Resizable && self->WidgetRep->GetInteractionState() != svtkBorderRepresentation::Inside)
  {
    return;
  }

  // Okay, adjust the representation (the widget is currently selected)
  double newEventPosition[2];
  newEventPosition[0] = static_cast<double>(X);
  newEventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(newEventPosition);

  // start a drag
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkBorderWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkBorderWidget* self = reinterpret_cast<svtkBorderWidget*>(w);

  if (self->SubclassEndSelectAction() ||
    self->WidgetRep->GetInteractionState() == svtkBorderRepresentation::Outside ||
    self->WidgetState != svtkBorderWidget::Selected)
  {
    return;
  }

  // Return state to not selected
  self->ReleaseFocus();
  self->WidgetState = svtkBorderWidget::Start;
  reinterpret_cast<svtkBorderRepresentation*>(self->WidgetRep)->MovingOff();

  // stop adjusting
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBorderWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkBorderRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkBorderWidget::SelectRegion(double* svtkNotUsed(eventPos[2]))
{
  this->InvokeEvent(svtkCommand::WidgetActivateEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkBorderWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Selectable: " << (this->Selectable ? "On\n" : "Off\n");
  os << indent << "Resizable: " << (this->Resizable ? "On\n" : "Off\n");
}
