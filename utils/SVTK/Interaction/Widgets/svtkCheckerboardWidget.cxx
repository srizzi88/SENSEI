/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCheckerboardWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCheckerboardWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCheckerboardRepresentation.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliderRepresentation3D.h"
#include "svtkSliderWidget.h"

svtkStandardNewMacro(svtkCheckerboardWidget);

// The checkerboard simply observes the behavior of four svtkSliderWidgets.
// Here we create the command/observer classes to respond to the
// slider widgets.
class svtkCWCallback : public svtkCommand
{
public:
  static svtkCWCallback* New() { return new svtkCWCallback; }
  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case svtkCommand::StartInteractionEvent:
        this->CheckerboardWidget->StartCheckerboardInteraction();
        break;
      case svtkCommand::InteractionEvent:
        this->CheckerboardWidget->CheckerboardInteraction(this->SliderNumber);
        break;
      case svtkCommand::EndInteractionEvent:
        this->CheckerboardWidget->EndCheckerboardInteraction();
        break;
    }
  }
  svtkCWCallback()
    : SliderNumber(0)
    , CheckerboardWidget(nullptr)
  {
  }
  int SliderNumber; // the number of the currently active slider
  svtkCheckerboardWidget* CheckerboardWidget;
};

//----------------------------------------------------------------------
svtkCheckerboardWidget::svtkCheckerboardWidget()
{
  this->TopSlider = svtkSliderWidget::New();
  this->TopSlider->KeyPressActivationOff();
  this->RightSlider = svtkSliderWidget::New();
  this->RightSlider->KeyPressActivationOff();
  this->BottomSlider = svtkSliderWidget::New();
  this->BottomSlider->KeyPressActivationOff();
  this->LeftSlider = svtkSliderWidget::New();
  this->LeftSlider->KeyPressActivationOff();

  // Set up the callbacks on the sliders
  svtkCWCallback* cwCallback0 = svtkCWCallback::New();
  cwCallback0->CheckerboardWidget = this;
  cwCallback0->SliderNumber = svtkCheckerboardRepresentation::TopSlider;
  this->TopSlider->AddObserver(svtkCommand::StartInteractionEvent, cwCallback0, this->Priority);
  this->TopSlider->AddObserver(svtkCommand::InteractionEvent, cwCallback0, this->Priority);
  this->TopSlider->AddObserver(svtkCommand::EndInteractionEvent, cwCallback0, this->Priority);
  cwCallback0->Delete(); // okay reference counting

  svtkCWCallback* cwCallback1 = svtkCWCallback::New();
  cwCallback1->CheckerboardWidget = this;
  cwCallback1->SliderNumber = svtkCheckerboardRepresentation::RightSlider;
  this->RightSlider->AddObserver(svtkCommand::StartInteractionEvent, cwCallback1, this->Priority);
  this->RightSlider->AddObserver(svtkCommand::InteractionEvent, cwCallback1, this->Priority);
  this->RightSlider->AddObserver(svtkCommand::EndInteractionEvent, cwCallback1, this->Priority);
  cwCallback1->Delete(); // okay reference counting

  svtkCWCallback* cwCallback2 = svtkCWCallback::New();
  cwCallback2->CheckerboardWidget = this;
  cwCallback2->SliderNumber = svtkCheckerboardRepresentation::BottomSlider;
  this->BottomSlider->AddObserver(svtkCommand::StartInteractionEvent, cwCallback2, this->Priority);
  this->BottomSlider->AddObserver(svtkCommand::InteractionEvent, cwCallback2, this->Priority);
  this->BottomSlider->AddObserver(svtkCommand::EndInteractionEvent, cwCallback2, this->Priority);
  cwCallback2->Delete(); // okay reference counting

  svtkCWCallback* cwCallback3 = svtkCWCallback::New();
  cwCallback3->CheckerboardWidget = this;
  cwCallback3->SliderNumber = svtkCheckerboardRepresentation::LeftSlider;
  this->LeftSlider->AddObserver(svtkCommand::StartInteractionEvent, cwCallback3, this->Priority);
  this->LeftSlider->AddObserver(svtkCommand::InteractionEvent, cwCallback3, this->Priority);
  this->LeftSlider->AddObserver(svtkCommand::EndInteractionEvent, cwCallback3, this->Priority);
  cwCallback3->Delete(); // okay reference counting
}

//----------------------------------------------------------------------
svtkCheckerboardWidget::~svtkCheckerboardWidget()
{
  this->TopSlider->Delete();
  this->RightSlider->Delete();
  this->BottomSlider->Delete();
  this->LeftSlider->Delete();
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkCheckerboardRepresentation::New();
  }
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //----------------
  {
    svtkDebugMacro(<< "Enabling checkerboard widget");

    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0], this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    // Everything is ok, enable the representation
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // Configure this slider widgets
    this->TopSlider->SetInteractor(this->Interactor);
    this->RightSlider->SetInteractor(this->Interactor);
    this->BottomSlider->SetInteractor(this->Interactor);
    this->LeftSlider->SetInteractor(this->Interactor);

    // Make sure there is a representation
    this->WidgetRep->BuildRepresentation();
    this->TopSlider->SetRepresentation(
      reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep)->GetTopRepresentation());
    this->RightSlider->SetRepresentation(
      reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep)->GetRightRepresentation());
    this->BottomSlider->SetRepresentation(
      reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep)->GetBottomRepresentation());
    this->LeftSlider->SetRepresentation(
      reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep)->GetLeftRepresentation());

    // We temporarily disable the sliders to avoid multiple renders
    this->Interactor->Disable();
    this->TopSlider->SetEnabled(1);
    this->RightSlider->SetEnabled(1);
    this->BottomSlider->SetEnabled(1);
    this->LeftSlider->SetEnabled(1);
    this->Interactor->Enable();

    // Add the actors
    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling------------------
  {
    svtkDebugMacro(<< "Disabling checkerboard widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // Turn off the slider widgets. Temporariliy disable
    this->Interactor->Disable();
    this->TopSlider->SetEnabled(0);
    this->RightSlider->SetEnabled(0);
    this->BottomSlider->SetEnabled(0);
    this->LeftSlider->SetEnabled(0);
    this->Interactor->Enable();

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
  }

  this->Render();
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::StartCheckerboardInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::CheckerboardInteraction(int sliderNum)
{
  reinterpret_cast<svtkCheckerboardRepresentation*>(this->WidgetRep)->SliderValueChanged(sliderNum);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::EndCheckerboardInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void svtkCheckerboardWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->TopSlider)
  {
    os << indent << "Top Slider: " << this->TopSlider << "\n";
  }
  else
  {
    os << indent << "Top Slider: (none)\n";
  }

  if (this->BottomSlider)
  {
    os << indent << "Bottom Slider: " << this->BottomSlider << "\n";
  }
  else
  {
    os << indent << "Bottom Slider: (none)\n";
  }

  if (this->BottomSlider)
  {
    os << indent << "Bottom Slider: " << this->BottomSlider << "\n";
  }
  else
  {
    os << indent << "Bottom Slider: (none)\n";
  }

  if (this->LeftSlider)
  {
    os << indent << "Left Slider: " << this->LeftSlider << "\n";
  }
  else
  {
    os << indent << "Left Slider: (none)\n";
  }
}
