/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLightWidget.h"

#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkLightRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkLightWidget);

//----------------------------------------------------------------------------
svtkLightWidget::svtkLightWidget()
{
  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkLightWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkLightWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkLightWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale, this, svtkLightWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkLightWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void svtkLightWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkLightWidget* self = svtkLightWidget::SafeDownCast(w);
  if (self->WidgetRep->GetInteractionState() == svtkLightRepresentation::Outside)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetActive = true;
  self->GrabFocus(self->EventCallbackCommand);
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  svtkLightRepresentation::SafeDownCast(self->WidgetRep)->StartWidgetInteraction(eventPosition);
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->StartInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void svtkLightWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkLightWidget* self = svtkLightWidget::SafeDownCast(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if (!self->WidgetActive)
  {
    self->Interactor->Disable(); // avoid extra renders

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == svtkLightRepresentation::Outside)
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else // must be near something
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_HAND);
    }
    self->Interactor->Enable(); // avoid extra renders
    if (changed || oldState != state)
    {
      self->Render();
    }
  }
  else // Already Active
  {
    // moving something
    double eventPosition[2];
    eventPosition[0] = static_cast<double>(X);
    eventPosition[1] = static_cast<double>(Y);
    svtkLightRepresentation::SafeDownCast(self->WidgetRep)->WidgetInteraction(eventPosition);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//----------------------------------------------------------------------
void svtkLightWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkLightWidget* self = svtkLightWidget::SafeDownCast(w);
  if (!self->WidgetActive)
  {
    return;
  }

  // Return state to not active
  self->WidgetActive = false;
  self->ReleaseFocus();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Superclass::EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkLightWidget::ScaleAction(svtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  svtkLightWidget* self = svtkLightWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetActive = false;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPosition);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState != svtkLightRepresentation::MovingPositionalFocalPoint)
  {
    return;
  }

  // We are definitely scaling the cone angle
  self->WidgetActive = true;
  self->GrabFocus(self->EventCallbackCommand);
  svtkLightRepresentation::SafeDownCast(self->WidgetRep)
    ->SetInteractionState(svtkLightRepresentation::ScalingConeAngle);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void svtkLightWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkLightRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkLightWidget::SetRepresentation(svtkLightRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//----------------------------------------------------------------------------
svtkLightRepresentation* svtkLightWidget::GetLightRepresentation()
{
  return svtkLightRepresentation::SafeDownCast(this->WidgetRep);
}

//----------------------------------------------------------------------------
void svtkLightWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "WidgetActive: " << this->WidgetActive << endl;
  this->Superclass::PrintSelf(os, indent);
}
