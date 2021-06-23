/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQWidgetWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQWidgetWidget.h"

#include <QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QWidget>

#include "svtkCallbackCommand.h"
// #include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkEventData.h"
#include "svtkObjectFactory.h"
#include "svtkQWidgetRepresentation.h"
#include "svtkQWidgetTexture.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
//#include "svtkStdString.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkQWidgetWidget);

//----------------------------------------------------------------------------
svtkQWidgetWidget::svtkQWidgetWidget()
{
  this->Widget = nullptr;
  this->WidgetState = svtkQWidgetWidget::Start;

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed, svtkWidgetEvent::Select3D,
      this, svtkQWidgetWidget::SelectAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::EndSelect3D, this, svtkQWidgetWidget::EndSelectAction3D);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D, this, svtkQWidgetWidget::MoveAction3D);
  }
}

//----------------------------------------------------------------------------
svtkQWidgetWidget::~svtkQWidgetWidget() {}

svtkQWidgetRepresentation* svtkQWidgetWidget::GetQWidgetRepresentation()
{
  return svtkQWidgetRepresentation::SafeDownCast(this->WidgetRep);
}

void svtkQWidgetWidget::SetWidget(QWidget* w)
{
  if (this->Widget == w)
  {
    return;
  }
  this->Widget = w;

  if (this->GetQWidgetRepresentation())
  {
    this->GetQWidgetRepresentation()->SetWidget(this->Widget);
  }
  this->Modified();
}

//-------------------------------------------------------------------------
void svtkQWidgetWidget::SelectAction3D(svtkAbstractWidget* w)
{
  svtkQWidgetWidget* self = reinterpret_cast<svtkQWidgetWidget*>(w);

  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == svtkQWidgetRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = svtkQWidgetWidget::Active;
  int widgetCoords[2];
  svtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
  Qt::MouseButton button = Qt::LeftButton;
  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMousePress);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setButtonDownPos(button, mousePos);
  mouseEvent.setButtonDownScenePos(button, ptGlobal);
  mouseEvent.setButtonDownScreenPos(button, ptGlobal);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(button);
  mouseEvent.setButton(button);
  mouseEvent.setModifiers(0);
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  self->EventCallbackCommand->SetAbortFlag(1);

  // fire a mouse click with the correct coords
  self->StartInteraction();
  self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkQWidgetWidget::MoveAction3D(svtkAbstractWidget* w)
{
  svtkQWidgetWidget* self = reinterpret_cast<svtkQWidgetWidget*>(w);

  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == svtkQWidgetRepresentation::Outside)
  {
    return;
  }

  int widgetCoords[2];
  svtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(
    self->WidgetState == svtkQWidgetWidget::Active ? Qt::LeftButton : Qt::NoButton);
  mouseEvent.setButton(Qt::NoButton);
  mouseEvent.setModifiers(0);
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);
  // OnSceneChanged( QList<QRectF>() );

  self->LastWidgetCoordinates = mousePos;

  self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkQWidgetWidget::EndSelectAction3D(svtkAbstractWidget* w)
{
  svtkQWidgetWidget* self = reinterpret_cast<svtkQWidgetWidget*>(w);

  if (self->WidgetState != svtkQWidgetWidget::Active ||
    self->WidgetRep->GetInteractionState() == svtkQWidgetRepresentation::Outside)
  {
    return;
  }

  self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, svtkWidgetEvent::Select3D, self->CallData);

  // We are definitely selected
  int widgetCoords[2];
  svtkQWidgetRepresentation* wrep = self->GetQWidgetRepresentation();
  wrep->GetWidgetCoordinates(widgetCoords);

  // if we are not mapped yet return
  QGraphicsScene* scene = wrep->GetQWidgetTexture()->GetScene();
  if (!scene)
  {
    return;
  }

  QPointF mousePos(widgetCoords[0], widgetCoords[1]);
  Qt::MouseButton button = Qt::LeftButton;
  QPoint ptGlobal = mousePos.toPoint();
  QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseRelease);
  mouseEvent.setWidget(nullptr);
  mouseEvent.setPos(mousePos);
  mouseEvent.setButtonDownPos(button, mousePos);
  mouseEvent.setButtonDownScenePos(button, ptGlobal);
  mouseEvent.setButtonDownScreenPos(button, ptGlobal);
  mouseEvent.setScenePos(ptGlobal);
  mouseEvent.setScreenPos(ptGlobal);
  mouseEvent.setLastPos(self->LastWidgetCoordinates);
  mouseEvent.setLastScenePos(ptGlobal);
  mouseEvent.setLastScreenPos(ptGlobal);
  mouseEvent.setButtons(Qt::NoButton);
  mouseEvent.setButton(button);
  mouseEvent.setModifiers(0);
  mouseEvent.setAccepted(false);

  QApplication::sendEvent(scene, &mouseEvent);

  self->LastWidgetCoordinates = mousePos;

  // Return state to not selected
  self->WidgetState = svtkQWidgetWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkQWidgetWidget::SetEnabled(int enabling)
{
  if (this->Enabled == enabling)
  {
    return;
  }

  if (enabling)
  {
    this->Widget->repaint();
  }
  Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void svtkQWidgetWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkQWidgetRepresentation::New();
    this->GetQWidgetRepresentation()->SetWidget(this->Widget);
  }
}

//----------------------------------------------------------------------
void svtkQWidgetWidget::SetRepresentation(svtkQWidgetRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<svtkWidgetRepresentation*>(rep));
  rep->SetWidget(this->Widget);
}

//----------------------------------------------------------------------------
void svtkQWidgetWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
