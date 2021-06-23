/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlaybackWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkPlaybackWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkPlaybackRepresentation.h"
#include "svtkPlaybackWidget.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

class svtkSubclassPlaybackRepresentation : public svtkPlaybackRepresentation
{
public:
  static svtkSubclassPlaybackRepresentation* New();
  void Play() override { std::cout << "play\n"; }
  void Stop() override { std::cout << "stop\n"; }
  void ForwardOneFrame() override { std::cout << "forward one frame\n"; }
  void BackwardOneFrame() override { std::cout << "backward one frame\n"; }
  void JumpToBeginning() override { std::cout << "jump to beginning\n"; }
  void JumpToEnd() override { std::cout << "jump to end\n"; }
};

svtkStandardNewMacro(svtkSubclassPlaybackRepresentation);

int TestPlaybackWidget(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  svtkSmartPointer<svtkSphereSource> ss = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Create the widget
  svtkSmartPointer<svtkSubclassPlaybackRepresentation> rep =
    svtkSmartPointer<svtkSubclassPlaybackRepresentation>::New();

  svtkSmartPointer<svtkPlaybackWidget> widget = svtkSmartPointer<svtkPlaybackWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
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

  return EXIT_SUCCESS;
}
