/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCenteredSliderWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkSliderWidget with a 2D representation.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkCenteredSliderWidget.h"
#include "svtkClipPolyData.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLODActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliderRepresentation2D.h"
#include "svtkSmartPointer.h"
#include "svtkSphere.h"
#include "svtkSphereSource.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

// This does the actual work: updates the probe.
// Callback for the interaction
class svtkCenteredSlider2DCallback : public svtkCommand
{
public:
  static svtkCenteredSlider2DCallback* New() { return new svtkCenteredSlider2DCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkCenteredSliderWidget* sliderWidget = reinterpret_cast<svtkCenteredSliderWidget*>(caller);
    double widgetValue = sliderWidget->GetValue();
    this->Glyph->SetScaleFactor(this->Glyph->GetScaleFactor() * widgetValue);
  }
  svtkCenteredSlider2DCallback()
    : Glyph(nullptr)
  {
  }
  svtkGlyph3D* Glyph;
};

int TestCenteredSliderWidget2D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkGlyph3D> glyph = svtkSmartPointer<svtkGlyph3D>::New();
  glyph->SetInputConnection(sphereSource->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  svtkSmartPointer<svtkAppendPolyData> apd = svtkSmartPointer<svtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphereSource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> maceMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  svtkSmartPointer<svtkLODActor> maceActor = svtkSmartPointer<svtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();
  maceActor->SetPosition(1, 1, 1);

  // Create the RenderWindow, Renderer and both Actors
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // SVTK widgets consist of two parts: the widget part that handles event
  // processing; and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  svtkSmartPointer<svtkSliderRepresentation2D> sliderRep =
    svtkSmartPointer<svtkSliderRepresentation2D>::New();
  sliderRep->SetMinimumValue(0.7);
  sliderRep->SetMaximumValue(1.3);
  sliderRep->SetValue(1.0);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.2, 0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.8, 0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.03);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);

  svtkSmartPointer<svtkCenteredSliderWidget> sliderWidget =
    svtkSmartPointer<svtkCenteredSliderWidget>::New();
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);

  svtkSmartPointer<svtkCenteredSlider2DCallback> callback =
    svtkSmartPointer<svtkCenteredSlider2DCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(svtkCommand::InteractionEvent, callback);

  ren1->AddActor(maceActor);

  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  //  recorder->ReadFromInputStringOn();
  //  recorder->SetInputString(eventLog);

  // render the image
  iren->Initialize();
  renWin->Render();
  //  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
