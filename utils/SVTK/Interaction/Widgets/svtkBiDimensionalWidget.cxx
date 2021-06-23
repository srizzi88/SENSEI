/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBiDimensionalWidget.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBiDimensionalWidget.h"
#include "svtkBiDimensionalRepresentation.h"
#include "svtkBiDimensionalRepresentation2D.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkHandleWidget.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkBiDimensionalWidget);

// The bidimensional widget observes the handles.
// Here we create the command/observer classes to respond to the
// slider widgets.
class svtkBiDimensionalWidgetCallback : public svtkCommand
{
public:
  static svtkBiDimensionalWidgetCallback* New() { return new svtkBiDimensionalWidgetCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::StartInteractionEvent:
        this->BiDimensionalWidget->StartBiDimensionalInteraction();
        break;
      case svtkCommand::EndInteractionEvent:
        this->BiDimensionalWidget->EndBiDimensionalInteraction();
        break;
    }
  }
  svtkBiDimensionalWidget* BiDimensionalWidget;
};

//----------------------------------------------------------------------
svtkBiDimensionalWidget::svtkBiDimensionalWidget()
{
  this->ManagesCursor = 1;

  this->WidgetState = svtkBiDimensionalWidget::Start;
  this->CurrentHandle = 0;

  // Manage priorities, we want the handles to be lower priority
  if (this->Priority <= 0.0)
  {
    this->Priority = 0.01;
  }

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = svtkHandleWidget::New();
  this->Point1Widget->SetPriority(this->Priority - 0.01);
  this->Point1Widget->SetParent(this);
  this->Point1Widget->ManagesCursorOff();

  this->Point2Widget = svtkHandleWidget::New();
  this->Point2Widget->SetPriority(this->Priority - 0.01);
  this->Point2Widget->SetParent(this);
  this->Point2Widget->ManagesCursorOff();

  this->Point3Widget = svtkHandleWidget::New();
  this->Point3Widget->SetPriority(this->Priority - 0.01);
  this->Point3Widget->SetParent(this);
  this->Point3Widget->ManagesCursorOff();

  this->Point4Widget = svtkHandleWidget::New();
  this->Point4Widget->SetPriority(this->Priority - 0.01);
  this->Point4Widget->SetParent(this);
  this->Point4Widget->ManagesCursorOff();

  // Set up the callbacks on the two handles
  this->BiDimensionalWidgetCallback1 = svtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback1->BiDimensionalWidget = this;
  this->Point1Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback1, this->Priority);
  this->Point1Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback1, this->Priority);

  this->BiDimensionalWidgetCallback2 = svtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback2->BiDimensionalWidget = this;
  this->Point2Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback2, this->Priority);
  this->Point2Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback2, this->Priority);

  this->BiDimensionalWidgetCallback3 = svtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback3->BiDimensionalWidget = this;
  this->Point3Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback3, this->Priority);
  this->Point3Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback3, this->Priority);

  this->BiDimensionalWidgetCallback4 = svtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback4->BiDimensionalWidget = this;
  this->Point4Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback4, this->Priority);
  this->Point4Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback4, this->Priority);

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkWidgetEvent::AddPoint, this, svtkBiDimensionalWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkBiDimensionalWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkBiDimensionalWidget::EndSelectAction);
  this->Line1InnerSelected = 0;
  this->Line1OuterSelected = 0;
  this->Line2InnerSelected = 0;
  this->Line2OuterSelected = 0;
  this->HandleLine1Selected = 0;
  this->HandleLine2Selected = 0;
  this->CenterSelected = 0;
}

//----------------------------------------------------------------------
svtkBiDimensionalWidget::~svtkBiDimensionalWidget()
{
  this->Point1Widget->RemoveObserver(this->BiDimensionalWidgetCallback1);
  this->Point1Widget->Delete();
  this->BiDimensionalWidgetCallback1->Delete();

  this->Point2Widget->RemoveObserver(this->BiDimensionalWidgetCallback2);
  this->Point2Widget->Delete();
  this->BiDimensionalWidgetCallback2->Delete();

  this->Point3Widget->RemoveObserver(this->BiDimensionalWidgetCallback3);
  this->Point3Widget->Delete();
  this->BiDimensionalWidgetCallback3->Delete();

  this->Point4Widget->RemoveObserver(this->BiDimensionalWidgetCallback4);
  this->Point4Widget->Delete();
  this->BiDimensionalWidgetCallback4->Delete();
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkBiDimensionalRepresentation2D::New();
  }
  svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the svtkBiDimensionalRepresentation.
  if (enabling)
  {
    if (this->WidgetState == svtkBiDimensionalWidget::Start)
    {
      if (this->WidgetRep)
      {
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->Line1VisibilityOff();
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->Line2VisibilityOff();
      }
    }
    else
    {
      if (this->WidgetRep)
      {
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->Line1VisibilityOn();
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->Line2VisibilityOn();
      }

      // The interactor must be set prior to enabling the widget.
      if (this->Interactor)
      {
        this->Point1Widget->SetInteractor(this->Interactor);
        this->Point2Widget->SetInteractor(this->Interactor);
        this->Point3Widget->SetInteractor(this->Interactor);
        this->Point4Widget->SetInteractor(this->Interactor);
      }

      this->Point1Widget->SetEnabled(1);
      this->Point2Widget->SetEnabled(1);
      this->Point3Widget->SetEnabled(1);
      this->Point4Widget->SetEnabled(1);
    }
  }

  if (enabling)
  {
    // Done in this weird order to get everything to work right.
    // This invocation creates the default representation.
    this->Superclass::SetEnabled(enabling);

    if (this->Point1Widget)
    {
      this->Point1Widget->SetRepresentation(
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->GetPoint1Representation());
      this->Point1Widget->SetInteractor(this->Interactor);
      this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }
    if (this->Point2Widget)
    {
      this->Point2Widget->SetRepresentation(
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->GetPoint2Representation());
      this->Point2Widget->SetInteractor(this->Interactor);
      this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }
    if (this->Point3Widget)
    {
      this->Point3Widget->SetRepresentation(
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->GetPoint3Representation());
      this->Point3Widget->SetInteractor(this->Interactor);
      this->Point3Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }
    if (this->Point4Widget)
    {
      this->Point4Widget->SetRepresentation(
        svtkBiDimensionalRepresentation::SafeDownCast(this->WidgetRep)->GetPoint4Representation());
      this->Point4Widget->SetInteractor(this->Interactor);
      this->Point4Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }
  }
  else // disabling widget
  {
    if (this->Point1Widget)
    {
      this->Point1Widget->SetEnabled(0);
    }
    if (this->Point2Widget)
    {
      this->Point2Widget->SetEnabled(0);
    }
    if (this->Point3Widget)
    {
      this->Point3Widget->SetEnabled(0);
    }
    if (this->Point4Widget)
    {
      this->Point4Widget->SetEnabled(0);
    }

    // Done in this weird order to get everything right. The renderer is
    // set to null after we disable the sub-widgets. That should give the
    // renderer a chance to remove the representation props before being
    // set to nullptr.
    this->Superclass::SetEnabled(enabling);
  }
}

//----------------------------------------------------------------------
int svtkBiDimensionalWidget::IsMeasureValid()
{
  if (this->WidgetState == svtkBiDimensionalWidget::Manipulate ||
    (this->WidgetState == svtkBiDimensionalWidget::Define && this->CurrentHandle == 2))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// The following methods are the callbacks that the bidimensional widget responds to.
//-------------------------------------------------------------------------
void svtkBiDimensionalWidget::AddPointAction(svtkAbstractWidget* w)
{
  svtkBiDimensionalWidget* self = svtkBiDimensionalWidget::SafeDownCast(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  // If we are placing the first point it's easy
  if (self->WidgetState == svtkBiDimensionalWidget::Start)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkBiDimensionalWidget::Define;
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->StartWidgetDefinition(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Line1VisibilityOn();
    self->Point1Widget->SetEnabled(1);
    self->CurrentHandle++;
  }

  // If defining we are placing the second or third point
  else if (self->WidgetState == svtkBiDimensionalWidget::Define)
  {
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    if (self->CurrentHandle == 1)
    {
      self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
      svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Point2WidgetInteraction(e);
      self->CurrentHandle++;
      self->Point2Widget->SetEnabled(1);
      self->Point3Widget->SetEnabled(1);
      self->Point4Widget->SetEnabled(1);
      svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Line2VisibilityOn();
    }
    else if (self->CurrentHandle == 2)
    {
      self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
      svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Point3WidgetInteraction(e);
      self->WidgetState = svtkBiDimensionalWidget::Manipulate;
      self->CurrentHandle = (-1);
      self->ReleaseFocus();
      self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
    }
  }

  // Maybe we are trying to manipulate the widget handles
  else // if ( self->WidgetState == svtkBiDimensionalWidget::Manipulate )
  {
    self->HandleLine1Selected = 0;
    self->HandleLine2Selected = 0;
    self->Line1InnerSelected = 0;
    self->Line1OuterSelected = 0;
    self->Line2InnerSelected = 0;
    self->Line2OuterSelected = 0;
    self->CenterSelected = 0;
    int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey();
    int state = self->WidgetRep->ComputeInteractionState(X, Y, modifier);
    if (state == svtkBiDimensionalRepresentation::Outside)
    {
      return;
    }

    self->GrabFocus(self->EventCallbackCommand);
    svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->StartWidgetManipulation(e);
    if (state == svtkBiDimensionalRepresentation::NearP1 ||
      state == svtkBiDimensionalRepresentation::NearP2)
    {
      self->HandleLine1Selected = 1;
      self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
    }
    else if (state == svtkBiDimensionalRepresentation::NearP3 ||
      state == svtkBiDimensionalRepresentation::NearP4)
    {
      self->HandleLine2Selected = 1;
      self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
    }
    else if (state == svtkBiDimensionalRepresentation::OnL1Inner)
    {
      self->WidgetRep->Highlight(1);
      self->Line1InnerSelected = 1;
      self->StartBiDimensionalInteraction();
    }
    else if (state == svtkBiDimensionalRepresentation::OnL1Outer)
    {
      self->WidgetRep->Highlight(1);
      self->Line1OuterSelected = 1;
      self->StartBiDimensionalInteraction();
    }
    else if (state == svtkBiDimensionalRepresentation::OnL2Inner)
    {
      self->WidgetRep->Highlight(1);
      self->Line2InnerSelected = 1;
      self->StartBiDimensionalInteraction();
    }
    else if (state == svtkBiDimensionalRepresentation::OnL2Outer)
    {
      self->WidgetRep->Highlight(1);
      self->Line2OuterSelected = 1;
      self->StartBiDimensionalInteraction();
    }
    else if (state == svtkBiDimensionalRepresentation::OnCenter)
    {
      self->WidgetRep->Highlight(1);
      self->CenterSelected = 1;
      self->StartBiDimensionalInteraction();
    }
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkBiDimensionalWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkBiDimensionalWidget* self = svtkBiDimensionalWidget::SafeDownCast(w);

  // Do nothing if outside
  if (self->WidgetState == svtkBiDimensionalWidget::Start)
  {
    return;
  }

  // Delegate the event consistent with the state
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  double p1[3], p2[3], slope;

  if (self->WidgetState == svtkBiDimensionalWidget::Define)
  {
    if (self->CurrentHandle == 1)
    {
      svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Point2WidgetInteraction(e);
    }
    else
    {
      svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->Point3WidgetInteraction(e);
    }
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
  }

  else if (self->Line1OuterSelected || self->Line2OuterSelected)
  {
    // moving outer portion of line -- rotating
    self->RequestCursorShape(SVTK_CURSOR_HAND);
    svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (self->Line1InnerSelected)
  { // must be moving inner portion of line 1 -- line translation
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint1DisplayPosition(p1);
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint2DisplayPosition(p2);
    slope = SVTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
    {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
    }
    if (slope > -1 && slope < 1)
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZENS);
    }
    else
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
    }

    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (self->Line2InnerSelected)
  { // must be moving inner portion of line 2 -- line translation
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint3DisplayPosition(p1);
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint4DisplayPosition(p2);
    slope = SVTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
    {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
    }
    if (slope > -1 && slope < 1)
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZENS);
    }
    else
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
    }

    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (self->HandleLine1Selected)
  { // moving one of the endpoints of line 1
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint1DisplayPosition(p1);
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint2DisplayPosition(p2);
    slope = SVTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
    {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
    }
    if (slope > -1 && slope < 1)
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
    }
    else
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZENS);
    }

    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (self->HandleLine2Selected)
  { // moving one of the endpoints of line 2
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint3DisplayPosition(p1);
    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
      ->GetPoint4DisplayPosition(p2);
    slope = SVTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
    {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
    }
    if (slope > -1 && slope < 1)
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
    }
    else
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZENS);
    }

    reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else if (self->CenterSelected)
  { // grabbing center intersection point
    self->RequestCursorShape(SVTK_CURSOR_SIZEALL);
    svtkBiDimensionalRepresentation::SafeDownCast(self->WidgetRep)->WidgetInteraction(e);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  else // just moving around, nothing yet selected
  {
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    if (state == svtkBiDimensionalRepresentation::Outside)
    {
      self->RequestCursorShape(SVTK_CURSOR_DEFAULT);
    }
    else if (state == svtkBiDimensionalRepresentation::OnCenter)
    {
      self->RequestCursorShape(SVTK_CURSOR_SIZEALL);
    }
    else if (state == svtkBiDimensionalRepresentation::NearP1 ||
      state == svtkBiDimensionalRepresentation::NearP2)
    {
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint1DisplayPosition(p1);
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint2DisplayPosition(p2);
      slope = SVTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
      {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
      if (slope > -1 && slope < 1)
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      }
      else
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZENS);
      }
    }
    else if (state == svtkBiDimensionalRepresentation::NearP3 ||
      state == svtkBiDimensionalRepresentation::NearP4)
    {
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint3DisplayPosition(p1);
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint4DisplayPosition(p2);
      slope = SVTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
      {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
      if (slope > -1 && slope < 1)
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      }
      else
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZENS);
      }
    }
    else if (state == svtkBiDimensionalRepresentation::OnL1Inner)
    {
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint1DisplayPosition(p1);
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint2DisplayPosition(p2);
      slope = SVTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
      {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
      if (slope > -1 && slope < 1)
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZENS);
      }
      else
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      }
    }
    else if (state == svtkBiDimensionalRepresentation::OnL2Inner)
    {
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint3DisplayPosition(p1);
      reinterpret_cast<svtkBiDimensionalRepresentation*>(self->WidgetRep)
        ->GetPoint4DisplayPosition(p2);
      slope = SVTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
      {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
      if (slope > -1 && slope < 1)
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZENS);
      }
      else
      {
        self->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      }
    }
    else
    {
      self->RequestCursorShape(SVTK_CURSOR_HAND);
    }
  }

  self->WidgetRep->BuildRepresentation();
  self->Render();
}

//-------------------------------------------------------------------------
void svtkBiDimensionalWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkBiDimensionalWidget* self = svtkBiDimensionalWidget::SafeDownCast(w);

  // Do nothing if outside
  if (self->WidgetState == svtkBiDimensionalWidget::Start ||
    self->WidgetState == svtkBiDimensionalWidget::Define ||
    (!self->HandleLine1Selected && !self->HandleLine2Selected && !self->Line1InnerSelected &&
      !self->Line1OuterSelected && !self->Line2InnerSelected && !self->Line2OuterSelected &&
      !self->CenterSelected))
  {
    return;
  }

  self->Line1InnerSelected = 0;
  self->Line1OuterSelected = 0;
  self->Line2InnerSelected = 0;
  self->Line2OuterSelected = 0;
  self->HandleLine1Selected = 0;
  self->HandleLine2Selected = 0;
  self->CenterSelected = 0;
  self->WidgetRep->Highlight(0);
  self->ReleaseFocus();
  self->CurrentHandle = (-1);
  self->WidgetRep->BuildRepresentation();
  int state = self->WidgetRep->GetInteractionState();
  if (state == svtkBiDimensionalRepresentation::NearP1 ||
    state == svtkBiDimensionalRepresentation::NearP2 ||
    state == svtkBiDimensionalRepresentation::NearP3 ||
    state == svtkBiDimensionalRepresentation::NearP4)
  {
    self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
  }
  else
  {
    self->EndBiDimensionalInteraction();
  }
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the angle widget.
//----------------------------------------------------------------------
void svtkBiDimensionalWidget::StartBiDimensionalInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::EndBiDimensionalInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->Point1Widget->SetProcessEvents(pe);
  this->Point2Widget->SetProcessEvents(pe);
  this->Point3Widget->SetProcessEvents(pe);
  this->Point4Widget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::SetWidgetStateToStart()
{
  this->WidgetState = svtkBiDimensionalWidget::Start;
  this->CurrentHandle = -1;
  this->HandleLine1Selected = 0;
  this->HandleLine2Selected = 0;
  this->Line1InnerSelected = 0;
  this->Line1OuterSelected = 0;
  this->Line2InnerSelected = 0;
  this->Line2OuterSelected = 0;
  this->CenterSelected = 0;
  this->SetEnabled(this->GetEnabled()); // show/hide the handles properly
  this->ReleaseFocus();
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::SetWidgetStateToManipulate()
{
  this->WidgetState = svtkBiDimensionalWidget::Manipulate;
  this->CurrentHandle = -1;
  this->HandleLine1Selected = 0;
  this->HandleLine2Selected = 0;
  this->Line1InnerSelected = 0;
  this->Line1OuterSelected = 0;
  this->Line2InnerSelected = 0;
  this->Line2OuterSelected = 0;
  this->CenterSelected = 0;
  this->SetEnabled(this->GetEnabled()); // show/hide the handles properly
  this->ReleaseFocus();
}

//----------------------------------------------------------------------
void svtkBiDimensionalWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
