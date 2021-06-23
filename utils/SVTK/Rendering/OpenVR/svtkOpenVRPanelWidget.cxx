/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRPanelWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRPanelWidget.h"
#include "svtkOpenVRPanelRepresentation.h"

#include "svtkCallbackCommand.h"
#include "svtkEventData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkOpenVRPanelWidget);

//----------------------------------------------------------------------
svtkOpenVRPanelWidget::svtkOpenVRPanelWidget()
{
  this->WidgetState = svtkOpenVRPanelWidget::Start;

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select3D,
      this, svtkOpenVRPanelWidget::SelectAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::EndSelect3D, this, svtkOpenVRPanelWidget::EndSelectAction3D);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D,
      this, svtkOpenVRPanelWidget::MoveAction3D);
  }
}

//----------------------------------------------------------------------
svtkOpenVRPanelWidget::~svtkOpenVRPanelWidget() {}

//----------------------------------------------------------------------
void svtkOpenVRPanelWidget::SetRepresentation(svtkOpenVRPanelRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
void svtkOpenVRPanelWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkOpenVRPanelRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkOpenVRPanelWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void svtkOpenVRPanelWidget::SelectAction3D(svtkAbstractWidget* w)
{
  svtkOpenVRPanelWidget* self = reinterpret_cast<svtkOpenVRPanelWidget*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == svtkOpenVRPanelRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = svtkOpenVRPanelWidget::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkOpenVRPanelWidget::MoveAction3D(svtkAbstractWidget* w)
{
  svtkOpenVRPanelWidget* self = reinterpret_cast<svtkOpenVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState == svtkOpenVRPanelWidget::Start)
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
void svtkOpenVRPanelWidget::EndSelectAction3D(svtkAbstractWidget* w)
{
  svtkOpenVRPanelWidget* self = reinterpret_cast<svtkOpenVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState != svtkOpenVRPanelWidget::Active ||
    self->WidgetRep->GetInteractionState() == svtkOpenVRPanelRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, svtkWidgetEvent::EndSelect3D, self->CallData);

  self->WidgetState = svtkOpenVRPanelWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}
