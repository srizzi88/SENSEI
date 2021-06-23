/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkHandleWidget with a 2D representation

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor2D.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkCursor2D.h"
#include "svtkDiskSource.h"
#include "svtkHandleWidget.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

// This does the actual work: updates the probe.
// Callback for the interaction
class svtkHandle2Callback : public svtkCommand
{
public:
  static svtkHandle2Callback* New() { return new svtkHandle2Callback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkHandleWidget* handleWidget = reinterpret_cast<svtkHandleWidget*>(caller);
    double pos[3];
    static_cast<svtkHandleRepresentation*>(handleWidget->GetRepresentation())
      ->GetDisplayPosition(pos);
    this->Actor->SetPosition(pos[0], pos[1]);
  }
  svtkHandle2Callback()
    : Actor(nullptr)
  {
  }
  svtkActor2D* Actor;
};

int TestHandleWidget2D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create two widgets
  //
  svtkSmartPointer<svtkDiskSource> diskSource = svtkSmartPointer<svtkDiskSource>::New();
  diskSource->SetInnerRadius(0.0);
  diskSource->SetOuterRadius(2);

  svtkSmartPointer<svtkPolyDataMapper2D> diskMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  diskMapper->SetInputConnection(diskSource->GetOutputPort());

  svtkSmartPointer<svtkActor2D> diskActor = svtkSmartPointer<svtkActor2D>::New();
  diskActor->SetMapper(diskMapper);
  diskActor->SetPosition(165, 180);

  svtkSmartPointer<svtkDiskSource> diskSource2 = svtkSmartPointer<svtkDiskSource>::New();
  diskSource2->SetInnerRadius(0.0);
  diskSource2->SetOuterRadius(2);

  svtkSmartPointer<svtkPolyDataMapper2D> diskMapper2 = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  diskMapper2->SetInputConnection(diskSource2->GetOutputPort());

  svtkSmartPointer<svtkActor2D> diskActor2 = svtkSmartPointer<svtkActor2D>::New();
  diskActor2->SetMapper(diskMapper2);
  diskActor2->SetPosition(50, 50);

  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The cursor shape can be defined externally. Here we use a default.
  svtkSmartPointer<svtkCursor2D> cursor2D = svtkSmartPointer<svtkCursor2D>::New();
  cursor2D->AllOff();
  cursor2D->AxesOn();
  cursor2D->OutlineOn();
  cursor2D->SetRadius(4);
  cursor2D->Update();

  svtkSmartPointer<svtkPointHandleRepresentation2D> handleRep =
    svtkSmartPointer<svtkPointHandleRepresentation2D>::New();
  handleRep->SetDisplayPosition(diskActor->GetPosition());
  handleRep->ActiveRepresentationOn();
  handleRep->SetCursorShape(cursor2D->GetOutput());

  svtkSmartPointer<svtkHandleWidget> handleWidget = svtkSmartPointer<svtkHandleWidget>::New();
  handleWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  svtkSmartPointer<svtkHandle2Callback> callback = svtkSmartPointer<svtkHandle2Callback>::New();
  callback->Actor = diskActor;
  handleWidget->AddObserver(svtkCommand::InteractionEvent, callback);

  svtkSmartPointer<svtkPointHandleRepresentation2D> handleRep2 =
    svtkSmartPointer<svtkPointHandleRepresentation2D>::New();
  handleRep2->SetDisplayPosition(diskActor2->GetPosition());
  //  handleRep2->ActiveRepresentationOn();
  handleRep2->SetCursorShape(cursor2D->GetOutput());

  svtkSmartPointer<svtkHandleWidget> handleWidget2 = svtkSmartPointer<svtkHandleWidget>::New();
  handleWidget2->SetInteractor(iren);
  handleWidget2->SetRepresentation(handleRep2);

  svtkSmartPointer<svtkHandle2Callback> callback2 = svtkSmartPointer<svtkHandle2Callback>::New();
  callback2->Actor = diskActor2;
  handleWidget2->AddObserver(svtkCommand::InteractionEvent, callback2);

  ren1->AddActor(diskActor);
  ren1->AddActor(diskActor2);

  // Add the actors to the renderer, set the background and size
  //
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
  //  recorder->Play();
  handleWidget->On();
  handleWidget2->On();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
