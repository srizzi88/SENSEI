/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProgressBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkProgressBarWidget.

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCylinderSource.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProgressBarRepresentation.h"
#include "svtkProgressBarWidget.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

int TestProgressBarWidget(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkNew<svtkRenderer> ren1;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create a test pipeline
  //
  svtkNew<svtkSphereSource> ss;
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkNew<svtkActor> sph;
  sph->SetMapper(mapper);

  svtkNew<svtkCylinderSource> cs;
  svtkNew<svtkPolyDataMapper> csMapper;
  csMapper->SetInputConnection(cs->GetOutputPort());
  svtkNew<svtkActor> cyl;
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5, 0, 0);

  svtkNew<svtkConeSource> coneSource;
  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  svtkNew<svtkActor> cone;
  cone->SetMapper(coneMapper);
  cone->AddPosition(0, 5, 0);

  // Create the widget
  svtkNew<svtkProgressBarRepresentation> rep;

  svtkNew<svtkProgressBarWidget> widget;
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Create the widget
  svtkNew<svtkProgressBarWidget> widget2;
  widget2->SetInteractor(iren);
  widget2->CreateDefaultRepresentation();
  svtkProgressBarRepresentation* rep2 =
    svtkProgressBarRepresentation::SafeDownCast(widget2->GetRepresentation());

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  rep->SetProgressRate(0.4);
  rep->SetPosition(0.4, 0.4);
  rep->SetProgressBarColor(0.2, 0.4, 0);
  rep->SetBackgroundColor(1, 1, 0.5);
  rep->DrawBackgroundOff();

  rep2->SetProgressRate(0.8);
  rep2->SetProgressBarColor(0.1, 0.8, 0);
  rep2->SetBackgroundColor(1, 1, 0.5);
  rep2->DrawBackgroundOn();

  renWin->Render();
  widget->On();
  widget2->On();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
