/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkTextWidget

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkCoordinate.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"
#include "svtkTextRepresentation.h"
#include "svtkTextWidget.h"

int TestTextWidget(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
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
  svtkSmartPointer<svtkTextActor> ta = svtkSmartPointer<svtkTextActor>::New();
  ta->SetInput("This is a test");
  ta->GetTextProperty()->SetColor(0.0, 1.0, 0.0);

  svtkSmartPointer<svtkTextWidget> widget = svtkSmartPointer<svtkTextWidget>::New();

  svtkSmartPointer<svtkTextRepresentation> rep = svtkSmartPointer<svtkTextRepresentation>::New();
  rep->GetPositionCoordinate()->SetValue(.15, .15);
  rep->GetPosition2Coordinate()->SetValue(.7, .2);
  widget->SetRepresentation(rep);

  widget->SetInteractor(iren);
  widget->SetTextActor(ta);
  widget->SelectableOff();

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
  renWin->Render();
  //  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
