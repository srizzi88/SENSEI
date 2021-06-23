/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSliderWidgetMultipleViewports.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkSliderWidget with multiple viewports.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkClipPolyData.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkGlyph3D.h"
#include "svtkLODActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSliderRepresentation2D.h"
#include "svtkSliderRepresentation3D.h"
#include "svtkSliderWidget.h"
#include "svtkSphere.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"
#include "svtkWidgetEvent.h"
#include "svtkWidgetEventTranslator.h"

const char TestSliderWidgetMultipleViewportsLog[] = "# StreamVersion 1\n"
                                                    "EnterEvent 292 46 0 0 0 0 0\n"
                                                    "MouseMoveEvent 273 65 0 0 0 0 0\n"
                                                    "MouseMoveEvent 252 88 0 0 0 0 0\n"
                                                    "MouseMoveEvent 148 299 0 0 0 0 0\n"
                                                    "LeaveEvent 147 301 0 0 0 0 0\n"
                                                    "EnterEvent 145 299 0 0 0 0 0\n"
                                                    "MouseMoveEvent 145 299 0 0 0 0 0\n"
                                                    "MouseMoveEvent 115 190 0 0 0 0 0\n"
                                                    "LeftButtonPressEvent 115 190 0 0 0 0 0\n"
                                                    "StartInteractionEvent 115 190 0 0 0 0 0\n"
                                                    "LeftButtonReleaseEvent 115 190 0 0 0 0 0\n"
                                                    "EndInteractionEvent 115 190 0 0 0 0 0\n"
                                                    "RenderEvent 115 190 0 0 0 0 0\n"
                                                    "KeyPressEvent 115 190 0 0 114 1 r\n"
                                                    "CharEvent 115 190 0 0 114 1 r\n"
                                                    "RenderEvent 115 190 0 0 114 1 r\n"
                                                    "KeyReleaseEvent 115 190 0 0 114 1 r\n"
                                                    "MouseMoveEvent 194 163 0 0 0 0 r\n"
                                                    "MouseMoveEvent 195 163 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 195 163 0 0 0 0 r\n"
                                                    "RenderEvent 195 163 0 0 0 0 r\n"
                                                    "MouseMoveEvent 195 163 0 0 0 0 r\n"
                                                    "MouseMoveEvent 201 151 0 0 0 0 r\n"
                                                    "RenderEvent 201 151 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 201 151 0 0 0 0 r\n"
                                                    "RenderEvent 201 151 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 204 29 0 0 0 0 r\n"
                                                    "RenderEvent 204 29 0 0 0 0 r\n"
                                                    "RenderEvent 210 30 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 210 30 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 158 159 0 0 0 0 r\n"
                                                    "RenderEvent 158 159 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 169 138 0 0 0 0 r\n"
                                                    "RenderEvent 169 138 0 0 0 0 r\n"
                                                    "RenderEvent 169 138 0 0 0 0 r\n"
                                                    "MouseMoveEvent 251 159 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 251 159 0 0 0 0 r\n"
                                                    "StartInteractionEvent 251 159 0 0 0 0 r\n"
                                                    "TimerEvent 251 159 0 0 0 0 r\n"
                                                    "RenderEvent 251 159 0 0 0 0 r\n"
                                                    "TimerEvent 251 159 0 0 0 0 r\n"
                                                    "RenderEvent 251 159 0 0 0 0 r\n"
                                                    "TimerEvent 251 159 0 0 0 0 r\n"
                                                    "RenderEvent 251 159 0 0 0 0 r\n"
                                                    "TimerEvent 251 159 0 0 0 0 r\n"
                                                    "RenderEvent 251 159 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 251 159 0 0 0 0 r\n"
                                                    "EndInteractionEvent 251 159 0 0 0 0 r\n"
                                                    "RenderEvent 251 159 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 250 159 0 0 0 0 r\n"
                                                    "StartInteractionEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "TimerEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 250 159 0 0 0 0 r\n"
                                                    "EndInteractionEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 250 159 0 0 0 0 r\n"
                                                    "RenderEvent 250 159 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 209 30 0 0 0 0 r\n"
                                                    "RenderEvent 209 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 209 30 0 0 0 0 r\n"
                                                    "RenderEvent 209 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 210 30 0 0 0 0 r\n"
                                                    "RenderEvent 210 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 210 30 0 0 0 0 r\n"
                                                    "RenderEvent 210 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 211 30 0 0 0 0 r\n"
                                                    "RenderEvent 211 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 212 30 0 0 0 0 r\n"
                                                    "RenderEvent 212 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 214 30 0 0 0 0 r\n"
                                                    "RenderEvent 214 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 214 30 0 0 0 0 r\n"
                                                    "RenderEvent 214 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 215 30 0 0 0 0 r\n"
                                                    "RenderEvent 215 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 233 30 0 0 0 0 r\n"
                                                    "RenderEvent 233 30 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 233 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 204 30 0 0 0 0 r\n"
                                                    "LeftButtonPressEvent 204 30 0 0 0 0 r\n"
                                                    "RenderEvent 204 30 0 0 0 0 r\n"
                                                    "LeftButtonReleaseEvent 204 30 0 0 0 0 r\n"
                                                    "RenderEvent 204 30 0 0 0 0 r\n"
                                                    "RenderEvent 204 30 0 0 0 0 r\n"
                                                    "MouseMoveEvent 239 83 0 0 0 0 r\n";

// This does the actual work: updates the probe.
// Callback for the interaction
class svtkSliderMultipleViewportsCallback : public svtkCommand
{
public:
  static svtkSliderMultipleViewportsCallback* New()
  {
    return new svtkSliderMultipleViewportsCallback;
  }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkSliderWidget* sliderWidget = reinterpret_cast<svtkSliderWidget*>(caller);
    this->Glyph->SetScaleFactor(
      static_cast<svtkSliderRepresentation*>(sliderWidget->GetRepresentation())->GetValue());
  }
  svtkSliderMultipleViewportsCallback()
    : Glyph(nullptr)
  {
  }
  svtkGlyph3D* Glyph;
};

int TestSliderWidgetMultipleViewports(int argc, char* argv[])
{
  // Create a mace out of filters.
  //
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
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  ren1->SetViewport(0, 0, 0.5, 1.0);
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  svtkSmartPointer<svtkRenderer> ren2 = svtkSmartPointer<svtkRenderer>::New();
  ren2->SetViewport(0.5, 0, 1.0, 1.0);
  renWin->AddRenderer(ren2);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // SVTK widgets consist of two parts: the widget part that handles event
  // processing; and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  svtkSmartPointer<svtkSliderRepresentation2D> sliderRep =
    svtkSmartPointer<svtkSliderRepresentation2D>::New();
  sliderRep->SetValue(0.25);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.1, 0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.4, 0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.01);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);

  svtkSmartPointer<svtkSliderWidget> sliderWidget = svtkSmartPointer<svtkSliderWidget>::New();
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetCurrentRenderer(ren2);
  sliderWidget->SetAnimationModeToAnimate();

  svtkSmartPointer<svtkSliderMultipleViewportsCallback> callback =
    svtkSmartPointer<svtkSliderMultipleViewportsCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(svtkCommand::InteractionEvent, callback);
  ren1->AddActor(maceActor);
  sliderWidget->EnabledOn();

  svtkSmartPointer<svtkSliderRepresentation3D> sliderRep3D =
    svtkSmartPointer<svtkSliderRepresentation3D>::New();
  sliderRep3D->SetValue(0.25);
  sliderRep3D->SetTitleText("Spike Size");
  sliderRep3D->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  sliderRep3D->GetPoint1Coordinate()->SetValue(0, 0, 0);
  sliderRep3D->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  sliderRep3D->GetPoint2Coordinate()->SetValue(2, 0, 0);
  sliderRep3D->SetSliderLength(0.075);
  sliderRep3D->SetSliderWidth(0.05);
  sliderRep3D->SetEndCapLength(0.05);

  svtkSmartPointer<svtkSliderWidget> sliderWidget3D = svtkSmartPointer<svtkSliderWidget>::New();
  sliderWidget3D->GetEventTranslator()->SetTranslation(
    svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Select);
  sliderWidget3D->GetEventTranslator()->SetTranslation(
    svtkCommand::RightButtonReleaseEvent, svtkWidgetEvent::EndSelect);
  sliderWidget3D->SetInteractor(iren);
  sliderWidget3D->SetRepresentation(sliderRep3D);
  sliderWidget3D->SetCurrentRenderer(ren2);
  sliderWidget3D->SetAnimationModeToAnimate();
  sliderWidget3D->EnabledOn();

  sliderWidget3D->AddObserver(svtkCommand::InteractionEvent, callback);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren2->SetBackground(0.9, 0.4, 0.2);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  return svtkTesting::InteractorEventLoop(argc, argv, iren, TestSliderWidgetMultipleViewportsLog);
}
