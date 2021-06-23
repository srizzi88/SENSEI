/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAxesTransformWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkCaptionWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkAxesTransformRepresentation.h"
#include "svtkAxesTransformWidget.h"
#include "svtkCommand.h"
#include "svtkDebugLeaks.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"

const char eventLog[] = "o";

int TestAxesTransformWidget(int, char*[])
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
  ss->SetCenter(100, 250, 500);
  ss->Update();

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Create the widget
  svtkSmartPointer<svtkAxesTransformRepresentation> rep =
    svtkSmartPointer<svtkAxesTransformRepresentation>::New();

  svtkSmartPointer<svtkAxesTransformWidget> widget = svtkSmartPointer<svtkAxesTransformWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Print the widget and its representation
  rep->Print(cout);
  widget->Print(cout);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
#endif

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  renWin->Render();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif
  iren->Start();

  return EXIT_SUCCESS;
}
