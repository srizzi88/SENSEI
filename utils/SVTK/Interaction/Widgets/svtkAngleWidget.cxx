/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAngleWidget.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAngleWidget.h"
#include "svtkAngleRepresentation2D.h"
#include "svtkCallbackCommand.h"
#include "svtkCoordinate.h"
#include "svtkHandleRepresentation.h"
#include "svtkHandleWidget.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkAngleWidget);

// The angle widget observes the handles.
// Here we create the command/observer classes to respond to the
// slider widgets.
class svtkAngleWidgetCallback : public svtkCommand
{
public:
  static svtkAngleWidgetCallback* New() { return new svtkAngleWidgetCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::StartInteractionEvent:
        this->AngleWidget->StartAngleInteraction(this->HandleNumber);
        break;
      case svtkCommand::InteractionEvent:
        this->AngleWidget->AngleInteraction(this->HandleNumber);
        break;
      case svtkCommand::EndInteractionEvent:
        this->AngleWidget->EndAngleInteraction(this->HandleNumber);
        break;
    }
  }
  int HandleNumber;
  svtkAngleWidget* AngleWidget;
};

//----------------------------------------------------------------------
svtkAngleWidget::svtkAngleWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = svtkAngleWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = svtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->CenterWidget = svtkHandleWidget::New();
  this->CenterWidget->SetParent(this);
  this->Point2Widget = svtkHandleWidget::New();
  this->Point2Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->AngleWidgetCallback1 = svtkAngleWidgetCallback::New();
  this->AngleWidgetCallback1->HandleNumber = 0;
  this->AngleWidgetCallback1->AngleWidget = this;
  this->Point1Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->AngleWidgetCallback1, this->Priority);
  this->Point1Widget->AddObserver(
    svtkCommand::InteractionEvent, this->AngleWidgetCallback1, this->Priority);
  this->Point1Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->AngleWidgetCallback1, this->Priority);

  // Set up the callbacks on the two handles
  this->AngleWidgetCenterCallback = svtkAngleWidgetCallback::New();
  this->AngleWidgetCenterCallback->HandleNumber = 1;
  this->AngleWidgetCenterCallback->AngleWidget = this;
  this->CenterWidget->AddObserver(
    svtkCommand::StartInteractionEvent, this->AngleWidgetCenterCallback, this->Priority);
  this->CenterWidget->AddObserver(
    svtkCommand::InteractionEvent, this->AngleWidgetCenterCallback, this->Priority);
  this->CenterWidget->AddObserver(
    svtkCommand::EndInteractionEvent, this->AngleWidgetCenterCallback, this->Priority);

  this->AngleWidgetCallback2 = svtkAngleWidgetCallback::New();
  this->AngleWidgetCallback2->HandleNumber = 2;
  this->AngleWidgetCallback2->AngleWidget = this;
  this->Point2Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->AngleWidgetCallback2, this->Priority);
  this->Point2Widget->AddObserver(
    svtkCommand::InteractionEvent, this->AngleWidgetCallback2, this->Priority);
  this->Point2Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->AngleWidgetCallback2, this->Priority);

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkWidgetEvent::AddPoint, this, svtkAngleWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkAngleWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkAngleWidget::EndSelectAction);
}

//----------------------------------------------------------------------
svtkAngleWidget::~svtkAngleWidget()
{
  this->Point1Widget->RemoveObserver(this->AngleWidgetCallback1);
  this->Point1Widget->Delete();
  this->AngleWidgetCallback1->Delete();

  this->CenterWidget->RemoveObserver(this->AngleWidgetCenterCallback);
  this->CenterWidget->Delete();
  this->AngleWidgetCenterCallback->Delete();

  this->Point2Widget->RemoveObserver(this->AngleWidgetCallback2);
  this->Point2Widget->Delete();
  this->AngleWidgetCallback2->Delete();
}

//----------------------------------------------------------------------
void svtkAngleWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkAngleRepresentation2D::New();
  }
  reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void svtkAngleWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the svtkAngleRepresentation.
  if (enabling)
  {
    if (this->WidgetState == svtkAngleWidget::Start)
    {
      if (this->WidgetRep)
      {
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->Ray1VisibilityOff();
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->Ray2VisibilityOff();
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->ArcVisibilityOff();
      }
    }
    else
    {
      if (this->WidgetRep)
      {
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->Ray1VisibilityOn();
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->Ray2VisibilityOn();
        reinterpret_cast<svtkAngleRepresentation*>(this->WidgetRep)->ArcVisibilityOn();
      }

      // The interactor must be set prior to enabling the widget.
      if (this->Interactor)
      {
        this->Point1Widget->SetInteractor(this->Interactor);
        this->CenterWidget->SetInteractor(this->Interactor);
        this->Point2Widget->SetInteractor(this->Interactor);
      }

      this->Point1Widget->SetEnabled(1);
      this->CenterWidget->SetEnabled(1);
      this->Point2Widget->SetEnabled(1);
    }
  }

  if (enabling) //----------------
  {
    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    if (!this->Interactor)
    {
      svtkErrorMacro(<< "The interactor must be set prior to enabling the widget");
      return;
    }

    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];

    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X, Y));

      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    // We're ready to enable
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // listen for the events found in the EventTranslator
    if (!this->Parent)
    {
      this->EventTranslator->AddEventsToInteractor(
        this->Interactor, this->EventCallbackCommand, this->Priority);
    }
    else
    {
      this->EventTranslator->AddEventsToParent(
        this->Parent, this->EventCallbackCommand, this->Priority);
    }

    if (this->ManagesCursor)
    {
      this->WidgetRep->ComputeInteractionState(X, Y);
      this->SetCursor(this->WidgetRep->GetInteractionState());
    }

    svtkAngleRepresentation* rep = static_cast<svtkAngleRepresentation*>(this->WidgetRep);

    // Set the renderer, representation and interactor on the child widgets.
    if (this->Point1Widget)
    {
      this->Point1Widget->SetRepresentation(rep->GetPoint1Representation());
      this->Point1Widget->SetInteractor(this->Interactor);
      this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }

    if (this->CenterWidget)
    {
      this->CenterWidget->SetRepresentation(rep->GetCenterRepresentation());
      this->CenterWidget->SetInteractor(this->Interactor);
      this->CenterWidget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }

    if (this->Point2Widget)
    {
      this->Point2Widget->SetRepresentation(rep->GetPoint2Representation());
      this->Point2Widget->SetInteractor(this->Interactor);
      this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }

    if (rep)
    {
      rep->SetRay1Visibility(this->WidgetState != svtkAngleWidget::Start ? 1 : 0);
      rep->SetRay2Visibility(this->WidgetState != svtkAngleWidget::Start ? 1 : 0);
      rep->SetArcVisibility(this->WidgetState != svtkAngleWidget::Start ? 1 : 0);
    }

    if (this->WidgetState != svtkAngleWidget::Start)
    {
      if (this->Point1Widget)
      {
        this->Point1Widget->SetEnabled(1);
      }
      if (this->CenterWidget)
      {
        this->CenterWidget->SetEnabled(1);
      }
      if (this->Point2Widget)
      {
        this->Point2Widget->SetEnabled(1);
      }
    }

    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling------------------
  {
    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // don't listen for events any more
    if (!this->Parent)
    {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
    else
    {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
    }

    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);

    if (svtkAngleRepresentation* rep = static_cast<svtkAngleRepresentation*>(this->WidgetRep))
    {
      rep->Ray1VisibilityOff();
      rep->Ray2VisibilityOff();
      rep->ArcVisibilityOff();
    }

    if (this->Point1Widget)
    {
      this->Point1Widget->SetEnabled(0);
    }

    if (this->CenterWidget)
    {
      this->CenterWidget->SetEnabled(0);
    }

    if (this->Point2Widget)
    {
      this->Point2Widget->SetEnabled(0);
    }

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
  }

  // Should only render if there is no parent
  if (this->Interactor && !this->Parent)
  {
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------
svtkTypeBool svtkAngleWidget::IsAngleValid()
{
  if (this->WidgetState == svtkAngleWidget::Manipulate ||
    (this->WidgetState == svtkAngleWidget::Define && this->CurrentHandle == 2))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// The following methods are the callbacks that the angle widget responds to.
//-------------------------------------------------------------------------
void svtkAngleWidget::AddPointAction(svtkAbstractWidget* w)
{
  svtkAngleWidget* self = reinterpret_cast<svtkAngleWidget*>(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // If we are placing the first point it's easy
  if (self->WidgetState == svtkAngleWidget::Start)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkAngleWidget::Define;
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->StartWidgetInteraction(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->Ray1VisibilityOn();
    self->Point1Widget->SetEnabled(1);
    self->CurrentHandle++;
  }

  // If defining we are placing the second or third point
  else if (self->WidgetState == svtkAngleWidget::Define)
  {
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    if (self->CurrentHandle == 1)
    {
      double e[2];
      e[0] = static_cast<double>(X);
      e[1] = static_cast<double>(Y);
      reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->CenterWidgetInteraction(e);
      self->CurrentHandle++;
      self->CenterWidget->SetEnabled(1);
      reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->Ray2VisibilityOn();
      reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->ArcVisibilityOn();
    }
    else if (self->CurrentHandle == 2)
    {
      self->WidgetState = svtkAngleWidget::Manipulate;
      self->Point2Widget->SetEnabled(1);
      self->CurrentHandle = (-1);
      self->ReleaseFocus();
      self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
    }
  }

  // Maybe we are trying to manipulate the widget handles
  else // if ( self->WidgetState == svtkAngleWidget::Manipulate )
  {
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    if (state == svtkAngleRepresentation::Outside)
    {
      self->CurrentHandle = (-1);
      return;
    }

    self->GrabFocus(self->EventCallbackCommand);
    if (state == svtkAngleRepresentation::NearP1)
    {
      self->CurrentHandle = 0;
    }
    else if (state == svtkAngleRepresentation::NearCenter)
    {
      self->CurrentHandle = 1;
    }
    else if (state == svtkAngleRepresentation::NearP2)
    {
      self->CurrentHandle = 2;
    }
    self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkAngleWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkAngleWidget* self = reinterpret_cast<svtkAngleWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState == svtkAngleWidget::Start)
  {
    return;
  }

  // Delegate the event consistent with the state
  if (self->WidgetState == svtkAngleWidget::Define)
  {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    if (self->CurrentHandle == 1)
    {
      reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->CenterWidgetInteraction(e);
    }
    else
    {
      reinterpret_cast<svtkAngleRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
    }
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
  }

  else // must be moving a handle, invoke a event for the handle widgets
  {
    self->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);
  }

  self->WidgetRep->BuildRepresentation();
  self->Render();
}

//-------------------------------------------------------------------------
void svtkAngleWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkAngleWidget* self = reinterpret_cast<svtkAngleWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState == svtkAngleWidget::Start || self->WidgetState == svtkAngleWidget::Define ||
    self->CurrentHandle < 0)
  {
    return;
  }

  self->ReleaseFocus();
  self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
  self->CurrentHandle = (-1);
  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the angle widget.
//----------------------------------------------------------------------
void svtkAngleWidget::StartAngleInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkAngleWidget::AngleInteraction(int)
{
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkAngleWidget::EndAngleInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkAngleWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  // Pass pe flag to component widgets.
  this->Point1Widget->SetProcessEvents(pe);
  this->CenterWidget->SetProcessEvents(pe);
  this->Point2Widget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------
void svtkAngleWidget::SetWidgetStateToStart()
{
  this->WidgetState = svtkAngleWidget::Start;
  this->CurrentHandle = -1;
  this->ReleaseFocus();
  this->GetRepresentation()->BuildRepresentation(); // update this->Angle
  this->SetEnabled(this->GetEnabled());             // show/hide the handles properly
}

//----------------------------------------------------------------------
void svtkAngleWidget::SetWidgetStateToManipulate()
{
  this->WidgetState = svtkAngleWidget::Manipulate;
  this->CurrentHandle = -1;
  this->ReleaseFocus();
  this->GetRepresentation()->BuildRepresentation(); // update this->Angle
  this->SetEnabled(this->GetEnabled());             // show/hide the handles properly
}

//----------------------------------------------------------------------
void svtkAngleWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
