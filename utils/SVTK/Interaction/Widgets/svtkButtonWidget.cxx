/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkButtonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkButtonWidget.h"
#include "svtkButtonRepresentation.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTexturedButtonRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkButtonWidget);

//----------------------------------------------------------------------------------
svtkButtonWidget::svtkButtonWidget()
{
  // Set the initial state
  this->WidgetState = svtkButtonWidget::Start;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkButtonWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkButtonWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkButtonWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void svtkButtonWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkTexturedButtonRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkButtonWidget::SetEnabled(int enabling)
{
  if (enabling) //----------------
  {
    if (this->Interactor)
    {
      if (!this->CurrentRenderer)
      {
        int X = this->Interactor->GetEventPosition()[0];
        int Y = this->Interactor->GetEventPosition()[1];
        this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X, Y));
      }
      this->CreateDefaultRepresentation();
      this->WidgetRep->SetRenderer(this->CurrentRenderer);
    }
  }
  else
  {
    this->SetCurrentRenderer(nullptr);
  }
  Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------------------
void svtkButtonWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkButtonWidget* self = reinterpret_cast<svtkButtonWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Motion while selecting is ignored
  if (self->WidgetState == svtkButtonWidget::Selecting)
  {
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }

  // Get the new state and compare it to the old
  int renderRequired = 0;
  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  if (self->WidgetState == svtkButtonWidget::Hovering)
  {
    if (state == svtkButtonRepresentation::Outside)
    {
      renderRequired = 1;
      if (self->ManagesCursor)
      {
        self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
      }
      self->WidgetRep->Highlight(svtkButtonRepresentation::HighlightNormal);
      self->WidgetState = svtkButtonWidget::Start;
    }
  }
  else // state is Start
  {
    if (state == svtkButtonRepresentation::Inside)
    {
      renderRequired = 1;
      if (self->ManagesCursor)
      {
        self->RequestCursorShape(SVTK_CURSOR_HAND);
      }
      self->WidgetRep->Highlight(svtkButtonRepresentation::HighlightHovering);
      self->WidgetState = svtkButtonWidget::Hovering;
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }

  if (renderRequired)
  {
    self->Render();
  }
}

//----------------------------------------------------------------------------------
void svtkButtonWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkButtonWidget* self = reinterpret_cast<svtkButtonWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // The state must be hovering for anything to happen. MoveAction sets the
  // state.
  if (self->WidgetState != svtkButtonWidget::Hovering)
  {
    return;
  }

  // Okay, make sure that the selection is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = svtkButtonWidget::Start;
    return;
  }

  // We are definitely selected, Highlight as necessary.
  self->WidgetState = svtkButtonWidget::Selecting;
  self->WidgetRep->Highlight(svtkButtonRepresentation::HighlightSelecting);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkButtonWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkButtonWidget* self = reinterpret_cast<svtkButtonWidget*>(w);

  if (self->WidgetState != svtkButtonWidget::Selecting)
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  if (state == svtkButtonRepresentation::Outside)
  {
    if (self->ManagesCursor)
    {
      self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    self->WidgetRep->Highlight(svtkButtonRepresentation::HighlightNormal);
    self->WidgetState = svtkButtonWidget::Start;
  }
  else // state == svtkButtonRepresentation::Inside
  {
    if (self->ManagesCursor)
    {
      self->RequestCursorShape(SVTK_CURSOR_HAND);
    }
    self->WidgetRep->Highlight(svtkButtonRepresentation::HighlightHovering);
    self->WidgetState = svtkButtonWidget::Hovering;
  }

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  reinterpret_cast<svtkButtonRepresentation*>(self->WidgetRep)->NextState();
  self->InvokeEvent(svtkCommand::StateChangedEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void svtkButtonWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
