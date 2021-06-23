/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCameraWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkCameraWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCameraRepresentation.h"
#include "svtkCameraWidget.h"
#include "svtkCommand.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"

int TestCameraWidget(int argc, char* argv[])
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
  svtkSmartPointer<svtkCameraRepresentation> rep = svtkSmartPointer<svtkCameraRepresentation>::New();
  rep->SetNumberOfFrames(2400);

  svtkSmartPointer<svtkCameraWidget> widget = svtkSmartPointer<svtkCameraWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  rep->SetCamera(ren1->GetActiveCamera());
  widget->On();

  int retVal = svtkTesting::InteractorEventLoop(argc, argv, iren);

  return retVal;
}
