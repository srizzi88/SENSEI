/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCaptionWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCaptionWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCaptionRepresentation.h"
#include "svtkCommand.h"
#include "svtkHandleWidget.h"
#include "svtkObjectFactory.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkCaptionWidget);

// The point widget invokes events that we watch for. Basically
// the attachment/anchor point is moved with the point widget.
class svtkCaptionAnchorCallback : public svtkCommand
{
public:
  static svtkCaptionAnchorCallback* New() { return new svtkCaptionAnchorCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::StartInteractionEvent:
        this->CaptionWidget->StartAnchorInteraction();
        break;
      case svtkCommand::InteractionEvent:
        this->CaptionWidget->AnchorInteraction();
        break;
      case svtkCommand::EndInteractionEvent:
        this->CaptionWidget->EndAnchorInteraction();
        break;
    }
  }
  svtkCaptionAnchorCallback()
    : CaptionWidget(nullptr)
  {
  }
  svtkCaptionWidget* CaptionWidget;
};

//-------------------------------------------------------------------------
svtkCaptionWidget::svtkCaptionWidget()
{
  // The priority of the point widget is set a little higher than me.
  // This is so Enable/Disable events are caught by the anchor and then
  // dispatched to the BorderWidget.
  this->HandleWidget = svtkHandleWidget::New();
  this->HandleWidget->SetPriority(this->Priority + 0.01);
  this->HandleWidget->KeyPressActivationOff();

  // over ride the call back mapper on the border widget superclass to move
  // the caption widget using the left mouse button (still moves on middle
  // mouse button press). Release is already mapped to end select action
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select,
    this, svtkBorderWidget::TranslateAction);

  this->AnchorCallback = svtkCaptionAnchorCallback::New();
  this->AnchorCallback->CaptionWidget = this;
  this->HandleWidget->AddObserver(svtkCommand::StartInteractionEvent, this->AnchorCallback, 1.0);
  this->HandleWidget->AddObserver(svtkCommand::InteractionEvent, this->AnchorCallback, 1.0);
  this->HandleWidget->AddObserver(svtkCommand::EndInteractionEvent, this->AnchorCallback, 1.0);
}

//-------------------------------------------------------------------------
svtkCaptionWidget::~svtkCaptionWidget()
{
  this->HandleWidget->Delete();
  this->AnchorCallback->Delete();
}

//----------------------------------------------------------------------
void svtkCaptionWidget::SetEnabled(int enabling)
{
  if (this->Interactor)
  {
    this->Interactor->Disable(); // avoid extra renders
  }

  if (enabling)
  {
    this->CreateDefaultRepresentation();
    this->HandleWidget->SetRepresentation(
      reinterpret_cast<svtkCaptionRepresentation*>(this->WidgetRep)->GetAnchorRepresentation());
    this->HandleWidget->SetInteractor(this->Interactor);
    this->HandleWidget->SetEnabled(1);
  }
  else
  {
    this->HandleWidget->SetEnabled(0);
  }
  if (this->Interactor)
  {
    this->Interactor->Enable();
  }

  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void svtkCaptionWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkCaptionRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkCaptionWidget::SetCaptionActor2D(svtkCaptionActor2D* capActor)
{
  svtkCaptionRepresentation* capRep = reinterpret_cast<svtkCaptionRepresentation*>(this->WidgetRep);
  if (!capRep)
  {
    this->CreateDefaultRepresentation();
    capRep = reinterpret_cast<svtkCaptionRepresentation*>(this->WidgetRep);
  }

  if (capRep->GetCaptionActor2D() != capActor)
  {
    capRep->SetCaptionActor2D(capActor);
    this->Modified();
  }
}

//-------------------------------------------------------------------------
svtkCaptionActor2D* svtkCaptionWidget::GetCaptionActor2D()
{
  svtkCaptionRepresentation* capRep = reinterpret_cast<svtkCaptionRepresentation*>(this->WidgetRep);
  if (!capRep)
  {
    return nullptr;
  }
  else
  {
    return capRep->GetCaptionActor2D();
  }
}

//----------------------------------------------------------------------
void svtkCaptionWidget::StartAnchorInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkCaptionWidget::AnchorInteraction()
{
  svtkCaptionRepresentation* rep = reinterpret_cast<svtkCaptionRepresentation*>(this->WidgetRep);
  double pos[3];
  rep->GetAnchorRepresentation()->GetWorldPosition(pos);
  rep->SetAnchorPosition(pos);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkCaptionWidget::EndAnchorInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkCaptionWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
