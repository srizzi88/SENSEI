/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTensorProbeWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTensorProbeWidget.h"

#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkEllipsoidTensorProbeRepresentation.h"
#include "svtkEvent.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkTensorProbeWidget);

//----------------------------------------------------------------------
svtkTensorProbeWidget::svtkTensorProbeWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkTensorProbeWidget::SelectAction);

  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkTensorProbeWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkTensorProbeWidget::MoveAction);

  this->Selected = 0;
}

//----------------------------------------------------------------------
svtkTensorProbeWidget::~svtkTensorProbeWidget() = default;

//----------------------------------------------------------------------
void svtkTensorProbeWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkEllipsoidTensorProbeRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkTensorProbeWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkTensorProbeWidget* self = reinterpret_cast<svtkTensorProbeWidget*>(w);

  if (!self->Selected)
  {
    svtkTensorProbeRepresentation* rep =
      reinterpret_cast<svtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);

    if (rep->SelectProbe(pos))
    {
      self->LastEventPosition[0] = pos[0];
      self->LastEventPosition[1] = pos[1];
      self->Selected = 1;
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }
}

//-------------------------------------------------------------------------
void svtkTensorProbeWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkTensorProbeWidget* self = reinterpret_cast<svtkTensorProbeWidget*>(w);

  if (self->Selected)
  {
    self->Selected = 0;
    self->EventCallbackCommand->SetAbortFlag(1);
    self->LastEventPosition[0] = -1;
    self->LastEventPosition[1] = -1;
  }
}

//-------------------------------------------------------------------------
void svtkTensorProbeWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkTensorProbeWidget* self = reinterpret_cast<svtkTensorProbeWidget*>(w);

  if (self->Selected)
  {
    svtkTensorProbeRepresentation* rep =
      reinterpret_cast<svtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);

    int delta0 = pos[0] - self->LastEventPosition[0];
    int delta1 = pos[1] - self->LastEventPosition[1];
    double motionVector[2] = { static_cast<double>(delta0), static_cast<double>(delta1) };

    self->LastEventPosition[0] = pos[0];
    self->LastEventPosition[1] = pos[1];

    if (rep->Move(motionVector))
    {
      self->EventCallbackCommand->SetAbortFlag(1);
      self->Render();
    }
  }
}

//----------------------------------------------------------------------
void svtkTensorProbeWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
