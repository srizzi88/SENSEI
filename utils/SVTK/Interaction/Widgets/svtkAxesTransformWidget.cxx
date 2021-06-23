/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxesTransformWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAxesTransformWidget.h"
#include "svtkAxesTransformRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkHandleWidget.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkAxesTransformWidget);

//----------------------------------------------------------------------------
svtkAxesTransformWidget::svtkAxesTransformWidget()
{
  this->WidgetState = svtkAxesTransformWidget::Start;
  this->ManagesCursor = 1;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->OriginWidget = svtkHandleWidget::New();
  this->OriginWidget->SetPriority(this->Priority - 0.01);
  this->OriginWidget->SetParent(this);
  this->OriginWidget->ManagesCursorOff();

  this->SelectionWidget = svtkHandleWidget::New();
  this->SelectionWidget->SetPriority(this->Priority - 0.01);
  this->SelectionWidget->SetParent(this);
  this->SelectionWidget->ManagesCursorOff();

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkAxesTransformWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkAxesTransformWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkAxesTransformWidget::MoveAction);
}

//----------------------------------------------------------------------------
svtkAxesTransformWidget::~svtkAxesTransformWidget()
{
  this->OriginWidget->Delete();
  this->SelectionWidget->Delete();
}

//----------------------------------------------------------------------
void svtkAxesTransformWidget::SetEnabled(int enabling)
{
  // We defer enabling the handles until the selection process begins
  if (enabling)
  {

    if (!this->CurrentRenderer)
    {
      int X = this->Interactor->GetEventPosition()[0];
      int Y = this->Interactor->GetEventPosition()[1];

      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X, Y));

      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    // Don't actually turn these on until cursor is near the end points or the line.
    this->CreateDefaultRepresentation();
    svtkHandleRepresentation* originRep =
      reinterpret_cast<svtkAxesTransformRepresentation*>(this->WidgetRep)->GetOriginRepresentation();
    originRep->SetRenderer(this->CurrentRenderer);
    this->OriginWidget->SetRepresentation(originRep);
    this->OriginWidget->SetInteractor(this->Interactor);

    svtkHandleRepresentation* selectionRep =
      reinterpret_cast<svtkAxesTransformRepresentation*>(this->WidgetRep)
        ->GetSelectionRepresentation();
    selectionRep->SetRenderer(this->CurrentRenderer);
    this->SelectionWidget->SetRepresentation(selectionRep);
    this->SelectionWidget->SetInteractor(this->Interactor);

    // We do this step first because it sets the CurrentRenderer
    this->Superclass::SetEnabled(enabling);
  }
  else
  {
    this->OriginWidget->SetEnabled(0);
    this->SelectionWidget->SetEnabled(0);
  }
}

//----------------------------------------------------------------------
void svtkAxesTransformWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkAxesTransformWidget* self = reinterpret_cast<svtkAxesTransformWidget*>(w);
  if (self->WidgetRep->GetInteractionState() == svtkAxesTransformRepresentation::Outside)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetState = svtkAxesTransformWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  reinterpret_cast<svtkAxesTransformRepresentation*>(self->WidgetRep)->StartWidgetInteraction(e);
  self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr); // for the handles
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void svtkAxesTransformWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkAxesTransformWidget* self = reinterpret_cast<svtkAxesTransformWidget*>(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if (self->WidgetState == svtkAxesTransformWidget::Start)
  {
    self->Interactor->Disable(); // avoid extra renders
    self->OriginWidget->SetEnabled(0);
    self->SelectionWidget->SetEnabled(0);

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == svtkAxesTransformRepresentation::Outside)
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else // must be near something
    {
      changed = self->RequestCursorShape(SVTK_CURSOR_HAND);
      if (state == svtkAxesTransformRepresentation::OnOrigin)
      {
        self->OriginWidget->SetEnabled(1);
      }
      else // if ( state == svtkAxesTransformRepresentation::OnXXX )
      {
        self->SelectionWidget->SetEnabled(1);
        changed = 1; // movement along the line always needs render
      }
    }
    self->Interactor->Enable(); // avoid extra renders
    if (changed || oldState != state)
    {
      self->Render();
    }
  }
  else // if ( self->WidgetState == svtkAxesTransformWidget::Active )
  {
    // moving something
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr); // handles observe this
    reinterpret_cast<svtkAxesTransformRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//----------------------------------------------------------------------
void svtkAxesTransformWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkAxesTransformWidget* self = reinterpret_cast<svtkAxesTransformWidget*>(w);
  if (self->WidgetState == svtkAxesTransformWidget::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = svtkAxesTransformWidget::Start;
  self->ReleaseFocus();
  self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr); // handles observe this
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  self->Superclass::EndInteraction();
  self->Render();
}

//----------------------------------------------------------------------
void svtkAxesTransformWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkAxesTransformRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void svtkAxesTransformWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->OriginWidget->SetProcessEvents(pe);
  this->SelectionWidget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------------
void svtkAxesTransformWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
