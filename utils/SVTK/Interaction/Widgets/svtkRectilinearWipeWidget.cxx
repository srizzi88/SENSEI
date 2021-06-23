/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearWipeWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearWipeWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearWipeRepresentation.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkRectilinearWipeWidget);

//----------------------------------------------------------------------
svtkRectilinearWipeWidget::svtkRectilinearWipeWidget()
{
  // Establish the initial widget state
  this->WidgetState = svtkRectilinearWipeWidget::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkRectilinearWipeWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkRectilinearWipeWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkRectilinearWipeWidget::MoveAction);
}

//----------------------------------------------------------------------
svtkRectilinearWipeWidget::~svtkRectilinearWipeWidget() = default;

//-------------------------------------------------------------------------
void svtkRectilinearWipeWidget::SetCursor(int cState)
{
  switch (cState)
  {
    case svtkRectilinearWipeRepresentation::MovingHPane:
      this->RequestCursorShape(SVTK_CURSOR_SIZENS);
      break;
    case svtkRectilinearWipeRepresentation::MovingVPane:
      this->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      break;
    case svtkRectilinearWipeRepresentation::MovingCenter:
      this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      break;
    default:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
}

//----------------------------------------------------------------------
void svtkRectilinearWipeWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkRectilinearWipeWidget* self = reinterpret_cast<svtkRectilinearWipeWidget*>(w);

  if (self->WidgetRep->GetInteractionState() == svtkRectilinearWipeRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkRectilinearWipeWidget::Selected;
  self->GrabFocus(self->EventCallbackCommand);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // We want to compute an orthogonal vector to the pane that has been selected
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkRectilinearWipeWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkRectilinearWipeWidget* self = reinterpret_cast<svtkRectilinearWipeWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState != svtkRectilinearWipeWidget::Selected)
  {
    self->WidgetRep->ComputeInteractionState(X, Y);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    return;
  }

  // Okay, adjust the representation
  double newEventPosition[2];
  newEventPosition[0] = static_cast<double>(X);
  newEventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(newEventPosition);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkRectilinearWipeWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkRectilinearWipeWidget* self = reinterpret_cast<svtkRectilinearWipeWidget*>(w);

  if (self->WidgetState != svtkRectilinearWipeWidget::Selected ||
    self->WidgetRep->GetInteractionState() == svtkRectilinearWipeRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetState = svtkRectilinearWipeWidget::Start;
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->WidgetState = svtkRectilinearWipeWidget::Start;
}

//----------------------------------------------------------------------
void svtkRectilinearWipeWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkRectilinearWipeRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkRectilinearWipeWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
