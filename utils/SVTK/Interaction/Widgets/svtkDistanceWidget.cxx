/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceWidget.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDistanceWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCoordinate.h"
#include "svtkDistanceRepresentation2D.h"
#include "svtkEventData.h"
#include "svtkHandleRepresentation.h"
#include "svtkHandleWidget.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

svtkStandardNewMacro(svtkDistanceWidget);

// The distance widget observes its two handles.
// Here we create the command/observer classes to respond to the
// handle widgets.
class svtkDistanceWidgetCallback : public svtkCommand
{
public:
  static svtkDistanceWidgetCallback* New() { return new svtkDistanceWidgetCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::StartInteractionEvent:
        this->DistanceWidget->StartDistanceInteraction(this->HandleNumber);
        break;
      case svtkCommand::InteractionEvent:
        this->DistanceWidget->DistanceInteraction(this->HandleNumber);
        break;
      case svtkCommand::EndInteractionEvent:
        this->DistanceWidget->EndDistanceInteraction(this->HandleNumber);
        break;
    }
  }
  int HandleNumber;
  svtkDistanceWidget* DistanceWidget;
};

//----------------------------------------------------------------------
svtkDistanceWidget::svtkDistanceWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = svtkDistanceWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = svtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->Point2Widget = svtkHandleWidget::New();
  this->Point2Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->DistanceWidgetCallback1 = svtkDistanceWidgetCallback::New();
  this->DistanceWidgetCallback1->HandleNumber = 0;
  this->DistanceWidgetCallback1->DistanceWidget = this;
  this->Point1Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->DistanceWidgetCallback1, this->Priority);
  this->Point1Widget->AddObserver(
    svtkCommand::InteractionEvent, this->DistanceWidgetCallback1, this->Priority);
  this->Point1Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->DistanceWidgetCallback1, this->Priority);

  this->DistanceWidgetCallback2 = svtkDistanceWidgetCallback::New();
  this->DistanceWidgetCallback2->HandleNumber = 1;
  this->DistanceWidgetCallback2->DistanceWidget = this;
  this->Point2Widget->AddObserver(
    svtkCommand::StartInteractionEvent, this->DistanceWidgetCallback2, this->Priority);
  this->Point2Widget->AddObserver(
    svtkCommand::InteractionEvent, this->DistanceWidgetCallback2, this->Priority);
  this->Point2Widget->AddObserver(
    svtkCommand::EndInteractionEvent, this->DistanceWidgetCallback2, this->Priority);

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent,
    svtkWidgetEvent::AddPoint, this, svtkDistanceWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkDistanceWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkDistanceWidget::EndSelectAction);

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::AddPoint3D, this, svtkDistanceWidget::AddPointAction3D);
  }

  {
    svtkNew<svtkEventDataButton3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    ed->SetInput(svtkEventDataDeviceInput::Trigger);
    ed->SetAction(svtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(svtkCommand::Button3DEvent, ed,
      svtkWidgetEvent::EndSelect3D, this, svtkDistanceWidget::EndSelectAction3D);
  }

  {
    svtkNew<svtkEventDataMove3D> ed;
    ed->SetDevice(svtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      svtkCommand::Move3DEvent, ed, svtkWidgetEvent::Move3D, this, svtkDistanceWidget::MoveAction3D);
  }
}

//----------------------------------------------------------------------
svtkDistanceWidget::~svtkDistanceWidget()
{
  this->Point1Widget->RemoveObserver(this->DistanceWidgetCallback1);
  this->Point1Widget->Delete();
  this->DistanceWidgetCallback1->Delete();

  this->Point2Widget->RemoveObserver(this->DistanceWidgetCallback2);
  this->Point2Widget->Delete();
  this->DistanceWidgetCallback2->Delete();
}

//----------------------------------------------------------------------
void svtkDistanceWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkDistanceRepresentation2D::New();
  }
  reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep)->InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void svtkDistanceWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the
  // svtkDistanceRepresentation.
  if (enabling)
  {
    if (this->WidgetState == svtkDistanceWidget::Start)
    {
      reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep)->VisibilityOff();
    }
    else
    {
      // The interactor must be set prior to enabling the widget.
      if (this->Interactor)
      {
        this->Point1Widget->SetInteractor(this->Interactor);
        this->Point2Widget->SetInteractor(this->Interactor);
      }

      this->Point1Widget->SetEnabled(1);
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

    // Set the renderer, interactor and representation on the two handle
    // widgets.
    this->Point1Widget->SetRepresentation(
      reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep)->GetPoint1Representation());
    this->Point1Widget->SetInteractor(this->Interactor);
    this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

    this->Point2Widget->SetRepresentation(
      reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep)->GetPoint2Representation());
    this->Point2Widget->SetInteractor(this->Interactor);
    this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

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

    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);

    if (this->WidgetState == svtkDistanceWidget::Start)
    {
      reinterpret_cast<svtkDistanceRepresentation*>(this->WidgetRep)->VisibilityOff();
    }
    else
    {
      this->Point1Widget->SetEnabled(1);
      this->Point2Widget->SetEnabled(1);
    }

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling------------------
  {
    svtkDebugMacro(<< "Disabling widget");

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

    this->Point1Widget->SetEnabled(0);
    this->Point2Widget->SetEnabled(0);

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
  }

  // Should only render if there is no parent
  if (this->Interactor && !this->Parent)
  {
    this->Interactor->Render();
  }
}

// The following methods are the callbacks that the measure widget responds to
//-------------------------------------------------------------------------
void svtkDistanceWidget::AddPointAction(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Freshly enabled and placing the first point
  if (self->WidgetState == svtkDistanceWidget::Start)
  {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = svtkDistanceWidget::Define;
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    reinterpret_cast<svtkDistanceRepresentation*>(self->WidgetRep)->VisibilityOn();
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<svtkDistanceRepresentation*>(self->WidgetRep)->StartWidgetInteraction(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
  }

  // Placing the second point is easy
  else if (self->WidgetState == svtkDistanceWidget::Define)
  {
    self->CurrentHandle = 1;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    self->WidgetState = svtkDistanceWidget::Manipulate;
    self->Point1Widget->SetEnabled(1);
    self->Point2Widget->SetEnabled(1);
    self->CurrentHandle = -1;
    self->ReleaseFocus();
    self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  }

  // Maybe we are trying to manipulate the widget handles
  else // if ( self->WidgetState == svtkDistanceWidget::Manipulate )
  {
    int state = self->WidgetRep->ComputeInteractionState(X, Y);

    if (state == svtkDistanceRepresentation::Outside)
    {
      self->CurrentHandle = -1;
      return;
    }

    self->GrabFocus(self->EventCallbackCommand);
    if (state == svtkDistanceRepresentation::NearP1)
    {
      self->CurrentHandle = 0;
    }
    else if (state == svtkDistanceRepresentation::NearP2)
    {
      self->CurrentHandle = 1;
    }
    self->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
  }

  // Clean up
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkDistanceWidget::AddPointAction3D(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);

  // Freshly enabled and placing the first point
  if (self->WidgetState == svtkDistanceWidget::Start)
  {
    self->WidgetState = svtkDistanceWidget::Define;
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    reinterpret_cast<svtkDistanceRepresentation*>(self->WidgetRep)->VisibilityOn();
    self->WidgetRep->StartComplexInteraction(
      self->Interactor, self, svtkWidgetEvent::AddPoint, self->CallData);
    self->CurrentHandle = 0;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    self->EventCallbackCommand->SetAbortFlag(1);
  }

  // Placing the second point is easy
  else if (self->WidgetState == svtkDistanceWidget::Define)
  {
    self->CurrentHandle = 1;
    self->InvokeEvent(svtkCommand::PlacePointEvent, &(self->CurrentHandle));
    self->WidgetState = svtkDistanceWidget::Manipulate;
    self->Point1Widget->SetEnabled(1);
    self->Point2Widget->SetEnabled(1);
    self->CurrentHandle = -1;
    self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
  }

  // Maybe we are trying to manipulate the widget handles
  else // if ( self->WidgetState == svtkDistanceWidget::Manipulate )
  {
    int state = self->WidgetRep->ComputeComplexInteractionState(
      self->Interactor, self, svtkWidgetEvent::AddPoint, self->CallData);

    if (state == svtkDistanceRepresentation::Outside)
    {
      self->CurrentHandle = -1;
      return;
    }

    if (state == svtkDistanceRepresentation::NearP1)
    {
      self->CurrentHandle = 0;
    }
    else if (state == svtkDistanceRepresentation::NearP2)
    {
      self->CurrentHandle = 1;
    }
    self->InvokeEvent(svtkCommand::Button3DEvent, self->CallData);
    self->EventCallbackCommand->SetAbortFlag(1);
  }

  // Clean up
  self->Render();
}

//-------------------------------------------------------------------------
void svtkDistanceWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);

  // Do nothing if in start mode or valid handle not selected
  if (self->WidgetState == svtkDistanceWidget::Start)
  {
    return;
  }

  // Delegate the event consistent with the state
  if (self->WidgetState == svtkDistanceWidget::Define)
  {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<svtkDistanceRepresentation*>(self->WidgetRep)->WidgetInteraction(e);
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
void svtkDistanceWidget::MoveAction3D(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);

  // Do nothing if in start mode or valid handle not selected
  if (self->WidgetState == svtkDistanceWidget::Start)
  {
    return;
  }

  // Delegate the event consistent with the state
  if (self->WidgetState == svtkDistanceWidget::Define)
  {
    self->WidgetRep->ComplexInteraction(
      self->Interactor, self, svtkWidgetEvent::Move3D, self->CallData);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }
  else // must be moving a handle, invoke a event for the handle widgets
  {
    self->InvokeEvent(svtkCommand::Move3DEvent, self->CallData);
  }

  self->WidgetRep->BuildRepresentation();
  self->Render();
}

//-------------------------------------------------------------------------
void svtkDistanceWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState == svtkDistanceWidget::Start ||
    self->WidgetState == svtkDistanceWidget::Define || self->CurrentHandle < 0)
  {
    return;
  }

  self->ReleaseFocus();
  self->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
  self->CurrentHandle = -1;
  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void svtkDistanceWidget::EndSelectAction3D(svtkAbstractWidget* w)
{
  svtkDistanceWidget* self = reinterpret_cast<svtkDistanceWidget*>(w);

  // Do nothing if outside
  if (self->WidgetState == svtkDistanceWidget::Start ||
    self->WidgetState == svtkDistanceWidget::Define || self->CurrentHandle < 0)
  {
    return;
  }

  self->ReleaseFocus();
  self->InvokeEvent(svtkCommand::Button3DEvent, self->CallData);
  self->CurrentHandle = -1;
  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the measure widget.
//----------------------------------------------------------------------
void svtkDistanceWidget::StartDistanceInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkDistanceWidget::DistanceInteraction(int)
{
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkDistanceWidget::EndDistanceInteraction(int)
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkDistanceWidget::SetProcessEvents(svtkTypeBool pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->Point1Widget->SetProcessEvents(pe);
  this->Point2Widget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------
void svtkDistanceWidget::SetWidgetStateToStart()
{
  this->WidgetState = svtkDistanceWidget::Start;
  this->CurrentHandle = -1;
  this->ReleaseFocus();
  this->GetRepresentation()->BuildRepresentation(); // update this->Distance
  this->SetEnabled(this->GetEnabled());             // show/hide the handles properly
}

//----------------------------------------------------------------------
void svtkDistanceWidget::SetWidgetStateToManipulate()
{
  this->WidgetState = svtkDistanceWidget::Manipulate;
  this->CurrentHandle = -1;
  this->ReleaseFocus();
  this->GetRepresentation()->BuildRepresentation(); // update this->Distance
  this->SetEnabled(this->GetEnabled());             // show/hide the handles properly
}

//----------------------------------------------------------------------
void svtkDistanceWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
