/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkScalarBarWidget.h"

#include "svtkCallbackCommand.h"
#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarBarRepresentation.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkScalarBarWidget);

//-------------------------------------------------------------------------
svtkScalarBarWidget::svtkScalarBarWidget()
{
  this->Selectable = 0;
  this->Repositionable = 1;

  // Override the subclasses callback to handle the Repositionable flag.
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkScalarBarWidget::MoveAction);
}

//-------------------------------------------------------------------------
svtkScalarBarWidget::~svtkScalarBarWidget() = default;

//-----------------------------------------------------------------------------
void svtkScalarBarWidget::SetRepresentation(svtkScalarBarRepresentation* rep)
{
  this->SetWidgetRepresentation(rep);
}

//-----------------------------------------------------------------------------
void svtkScalarBarWidget::SetScalarBarActor(svtkScalarBarActor* actor)
{
  svtkScalarBarRepresentation* rep = this->GetScalarBarRepresentation();
  if (!rep)
  {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
  }

  if (rep->GetScalarBarActor() != actor)
  {
    rep->SetScalarBarActor(actor);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkScalarBarActor* svtkScalarBarWidget::GetScalarBarActor()
{
  svtkScalarBarRepresentation* rep = this->GetScalarBarRepresentation();
  if (!rep)
  {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
  }

  return rep->GetScalarBarActor();
}

//-----------------------------------------------------------------------------
void svtkScalarBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    svtkScalarBarRepresentation* rep = svtkScalarBarRepresentation::New();
    this->SetRepresentation(rep);
    rep->Delete();
  }
}

//-------------------------------------------------------------------------
void svtkScalarBarWidget::SetCursor(int cState)
{
  if (!this->Repositionable && !this->Selectable && cState == svtkBorderRepresentation::Inside)
  {
    // Don't have a special cursor for the inside if we cannot reposition.
    this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  }
  else
  {
    this->Superclass::SetCursor(cState);
  }
}

//-------------------------------------------------------------------------
void svtkScalarBarWidget::MoveAction(svtkAbstractWidget* w)
{
  // The superclass handle most stuff.
  svtkScalarBarWidget::Superclass::MoveAction(w);

  svtkScalarBarWidget* self = reinterpret_cast<svtkScalarBarWidget*>(w);
  svtkScalarBarRepresentation* representation = self->GetScalarBarRepresentation();

  // Handle the case where we suppress widget translation.
  if (!self->Repositionable &&
    (representation->GetInteractionState() == svtkBorderRepresentation::Inside))
  {
    representation->MovingOff();
  }
}

//-------------------------------------------------------------------------
void svtkScalarBarWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Repositionable: " << this->Repositionable << endl;
}
