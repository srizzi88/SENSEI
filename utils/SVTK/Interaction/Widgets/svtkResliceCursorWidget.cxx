/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceCursorWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkImageData.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkInteractorStyleImage.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorLineRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkResliceCursorWidget);

//----------------------------------------------------------------------------
svtkResliceCursorWidget::svtkResliceCursorWidget()
{
  // Set the initial state
  this->WidgetState = svtkResliceCursorWidget::Start;

  this->ModifierActive = 0;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkEvent::NoModifier, 0,
    0, nullptr, svtkWidgetEvent::Select, this, svtkResliceCursorWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkEvent::ControlModifier, 0, 0, nullptr, svtkWidgetEvent::Rotate, this,
    svtkResliceCursorWidget::RotateAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkResliceCursorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Resize,
    this, svtkResliceCursorWidget::ResizeThicknessAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndResize, this, svtkResliceCursorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkResliceCursorWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::NoModifier, 111, 1,
    "o", svtkWidgetEvent::Reset, this, svtkResliceCursorWidget::ResetResliceCursorAction);

  this->ManageWindowLevel = 1;
}

//----------------------------------------------------------------------------
svtkResliceCursorWidget::~svtkResliceCursorWidget() = default;

//----------------------------------------------------------------------
void svtkResliceCursorWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void svtkResliceCursorWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkResliceCursorLineRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::SetCursor(int cState)
{
  switch (cState)
  {
    case svtkResliceCursorRepresentation::OnAxis1:
    case svtkResliceCursorRepresentation::OnAxis2:
      this->RequestCursorShape(SVTK_CURSOR_HAND);
      break;
    case svtkResliceCursorRepresentation::OnCenter:
      if (svtkEvent::GetModifier(this->Interactor) != svtkEvent::ControlModifier)
      {
        this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      }
      break;
    case svtkResliceCursorRepresentation::Outside:
    default:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::ResizeThicknessAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = reinterpret_cast<svtkResliceCursorWidget*>(w);
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if (self->WidgetRep->GetInteractionState() == svtkResliceCursorRepresentation::Outside ||
    rep->GetResliceCursor()->GetThickMode() == 0)
  {
    return;
  }

  rep->SetManipulationMode(svtkResliceCursorRepresentation::ResizeThickness);

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = svtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();

  self->InvokeAnEvent();

  // Show the thickness in "mm"
  rep->ActivateText(1);
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::EndResizeThicknessAction(svtkAbstractWidget*) {}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = reinterpret_cast<svtkResliceCursorWidget*>(w);
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = svtkEvent::GetModifier(self->Interactor);
  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if (self->WidgetRep->GetInteractionState() == svtkResliceCursorRepresentation::Outside)
  {
    if (self->GetManageWindowLevel() && rep->GetShowReslicedImage())
    {
      self->StartWindowLevel();
    }
    else
    {
      rep->SetManipulationMode(svtkResliceCursorRepresentation::None);
      return;
    }
  }
  else
  {
    rep->SetManipulationMode(svtkResliceCursorRepresentation::PanAndRotate);
  }

  if (rep->GetManipulationMode() == svtkResliceCursorRepresentation::None)
  {
    return;
  }

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = svtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::RotateAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = reinterpret_cast<svtkResliceCursorWidget*>(w);
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = svtkEvent::GetModifier(self->Interactor);
  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if (self->WidgetRep->GetInteractionState() == svtkResliceCursorRepresentation::Outside)
  {
    return;
  }

  rep->SetManipulationMode(svtkResliceCursorRepresentation::RotateBothAxes);

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = svtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = reinterpret_cast<svtkResliceCursorWidget*>(w);
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(self->WidgetRep);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState == svtkResliceCursorWidget::Start)
  {
    self->ModifierActive = svtkEvent::GetModifier(self->Interactor);
    int state = self->WidgetRep->GetInteractionState();

    rep->ComputeInteractionState(X, Y, self->ModifierActive);

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

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = static_cast<svtkResliceCursorWidget*>(w);
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(self->WidgetRep);

  if (self->WidgetState != svtkResliceCursorWidget::Active)
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
  self->WidgetState = svtkResliceCursorWidget::Start;
  self->ModifierActive = 0;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // Remove any text displays. We are no longer active.
  rep->ActivateText(0);

  // stop adjusting
  self->ReleaseFocus();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->WidgetState = svtkResliceCursorWidget::Start;
  rep->SetManipulationMode(svtkResliceCursorRepresentation::None);

  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::ResetResliceCursorAction(svtkAbstractWidget* w)
{
  svtkResliceCursorWidget* self = reinterpret_cast<svtkResliceCursorWidget*>(w);
  self->ResetResliceCursor();

  // Render in response to changes
  self->Render();

  // Invoke a reslice cursor event
  self->InvokeEvent(svtkResliceCursorWidget::ResetCursorEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkResliceCursorWidget::ResetResliceCursor()
{
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(this->WidgetRep);

  if (!rep->GetResliceCursor())
  {
    return; // nothing to reset
  }

  // Reset the reslice cursor
  rep->GetResliceCursor()->Reset();
  rep->InitializeReslicePlane();
}

//----------------------------------------------------------------------------
void svtkResliceCursorWidget::StartWindowLevel()
{
  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(this->WidgetRep);

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    rep->SetManipulationMode(svtkResliceCursorRepresentation::None);
    return;
  }

  rep->SetManipulationMode(svtkResliceCursorRepresentation::WindowLevelling);

  rep->ActivateText(1);
  rep->ManageTextDisplay();
}

//----------------------------------------------------------------------------
void svtkResliceCursorWidget::InvokeAnEvent()
{
  // We invoke the appropriate event. In cases where the cursor is moved
  // around, or rotated, also have the reslice cursor invoke an event.

  svtkResliceCursorRepresentation* rep =
    reinterpret_cast<svtkResliceCursorRepresentation*>(this->WidgetRep);
  if (rep)
  {
    int mode = rep->GetManipulationMode();
    if (mode == svtkResliceCursorRepresentation::WindowLevelling)
    {
      this->InvokeEvent(WindowLevelEvent, nullptr);
    }
    else if (mode == svtkResliceCursorRepresentation::PanAndRotate)
    {
      this->InvokeEvent(ResliceAxesChangedEvent, nullptr);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent, nullptr);
    }
    else if (mode == svtkResliceCursorRepresentation::RotateBothAxes)
    {
      this->InvokeEvent(ResliceAxesChangedEvent, nullptr);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent, nullptr);
    }
    else if (mode == svtkResliceCursorRepresentation::ResizeThickness)
    {
      this->InvokeEvent(ResliceThicknessChangedEvent, nullptr);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent, nullptr);
    }
  }
}

//----------------------------------------------------------------------------
void svtkResliceCursorWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ManageWindowLevel: " << this->ManageWindowLevel << endl;
  // this->ModifierActive;
  // this->WidgetState;
}
