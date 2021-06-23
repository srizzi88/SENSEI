/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBalloonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkHoverWidget and svtkBalloonWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkBalloonRepresentation.h"
#include "svtkBalloonWidget.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCylinderSource.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkPolyDataMapper.h"
#include "svtkPropPicker.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTIFFReader.h"
#include "svtkTestUtilities.h"

class svtkBalloonCallback : public svtkCommand
{
public:
  static svtkBalloonCallback* New() { return new svtkBalloonCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkBalloonWidget* balloonWidget = reinterpret_cast<svtkBalloonWidget*>(caller);
    if (balloonWidget->GetCurrentProp() != nullptr)
    {
      std::cout << "Prop selected\n";
    }
  }

  svtkActor* PickedActor;
};

class svtkBalloonPickCallback : public svtkCommand
{
public:
  static svtkBalloonPickCallback* New() { return new svtkBalloonPickCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkPropPicker* picker = reinterpret_cast<svtkPropPicker*>(caller);
    svtkProp* prop = picker->GetViewProp();
    if (prop != nullptr)
    {
      this->BalloonWidget->UpdateBalloonString(prop, "Picked");
    }
  }
  svtkBalloonWidget* BalloonWidget;
};

int TestBalloonWidget(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkPropPicker> picker = svtkSmartPointer<svtkPropPicker>::New();
  svtkSmartPointer<svtkBalloonPickCallback> pcbk = svtkSmartPointer<svtkBalloonPickCallback>::New();
  picker->AddObserver(svtkCommand::PickEvent, pcbk);
  iren->SetPicker(picker);

  // Create an image for the balloon widget
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  svtkSmartPointer<svtkTIFFReader> image1 = svtkSmartPointer<svtkTIFFReader>::New();
  image1->SetFileName(fname);
  image1->SetOrientationType(4);

  // Create a test pipeline
  //
  svtkSmartPointer<svtkSphereSource> ss = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkSmartPointer<svtkActor> sph = svtkSmartPointer<svtkActor>::New();
  sph->SetMapper(mapper);

  svtkSmartPointer<svtkCylinderSource> cs = svtkSmartPointer<svtkCylinderSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> csMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  csMapper->SetInputConnection(cs->GetOutputPort());
  svtkSmartPointer<svtkActor> cyl = svtkSmartPointer<svtkActor>::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5, 0, 0);

  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> coneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  svtkSmartPointer<svtkActor> cone = svtkSmartPointer<svtkActor>::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0, 5, 0);

  // Create the widget
  svtkSmartPointer<svtkBalloonRepresentation> rep = svtkSmartPointer<svtkBalloonRepresentation>::New();
  rep->SetBalloonLayoutToImageRight();

  svtkSmartPointer<svtkBalloonWidget> widget = svtkSmartPointer<svtkBalloonWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->AddBalloon(sph, "This is a sphere", nullptr);
  widget->AddBalloon(cyl, "This is a\ncylinder", image1->GetOutput());
  widget->AddBalloon(cone, "This is a\ncone,\na really big cone,\nyou wouldn't believe how big",
    image1->GetOutput());
  pcbk->BalloonWidget = widget;

  svtkSmartPointer<svtkBalloonCallback> cbk = svtkSmartPointer<svtkBalloonCallback>::New();
  widget->AddObserver(svtkCommand::WidgetActivateEvent, cbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
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
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  //  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  delete[] fname;

  return EXIT_SUCCESS;
}
