/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFinitePlaneWidget.cxx

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFinitePlaneWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkFinitePlaneRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkFinitePlaneWidget);

//----------------------------------------------------------------------------
svtkFinitePlaneWidget::svtkFinitePlaneWidget()
{
  this->WidgetState = svtkFinitePlaneWidget::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkFinitePlaneWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkFinitePlaneWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkFinitePlaneWidget::MoveAction);
}

//----------------------------------------------------------------------------
svtkFinitePlaneWidget::~svtkFinitePlaneWidget() = default;

//----------------------------------------------------------------------------
void svtkFinitePlaneWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkFinitePlaneWidget::SetRepresentation(svtkFinitePlaneRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//----------------------------------------------------------------------
void svtkFinitePlaneWidget::SelectAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkFinitePlaneWidget* self = reinterpret_cast<svtkFinitePlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<svtkFinitePlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(svtkFinitePlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == svtkFinitePlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = svtkFinitePlaneWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkFinitePlaneWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkFinitePlaneWidget* self = reinterpret_cast<svtkFinitePlaneWidget*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  svtkFinitePlaneRepresentation* repr =
    reinterpret_cast<svtkFinitePlaneRepresentation*>(self->WidgetRep);

  if (self->ManagesCursor && self->WidgetState != svtkFinitePlaneWidget::Active)
  {
    int oldInteractionState = repr->GetInteractionState();

    repr->SetInteractionState(svtkFinitePlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    changed = self->UpdateCursorShape(state);
    repr->SetInteractionState(oldInteractionState);
    changed = (changed || state != oldInteractionState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == svtkFinitePlaneWidget::Start)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

  // Adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // Moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkFinitePlaneWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkFinitePlaneWidget* self = reinterpret_cast<svtkFinitePlaneWidget*>(w);

  if (self->WidgetState != svtkFinitePlaneWidget::Active ||
    self->WidgetRep->GetInteractionState() == svtkFinitePlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = svtkFinitePlaneWidget::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<svtkFinitePlaneRepresentation*>(self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkFinitePlaneWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkFinitePlaneRepresentation::New();
  }
}

//----------------------------------------------------------------------
int svtkFinitePlaneWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == svtkFinitePlaneRepresentation::Outside)
    {
      return this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else
    {
      return this->RequestCursorShape(SVTK_CURSOR_HAND);
    }
  }

  return 0;
}
