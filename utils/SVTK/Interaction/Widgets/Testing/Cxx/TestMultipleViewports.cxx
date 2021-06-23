/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBorderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests laying out widgets in multiple viewports.

// First include the required header files for the SVTK classes we are using.
#include "svtkActor.h"
#include "svtkBorderRepresentation.h"
#include "svtkBorderWidget.h"
#include "svtkCommand.h"
#include "svtkHandleWidget.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPointHandleRepresentation2D.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestMultipleViewports(int, char*[])
{
  // Create the RenderWindow, Renderers
  //
  svtkNew<svtkRenderer> ren0;
  svtkNew<svtkRenderer> ren1;
  svtkNew<svtkRenderWindow> renWin;

  ren0->SetBackground(0, 0, 0);
  ren0->SetViewport(0, 0, 0.5, 1);
  ren1->SetBackground(0.1, 0.1, 0.1);
  ren1->SetViewport(0.5, 0, 1, 1);

  renWin->AddRenderer(ren0);
  renWin->AddRenderer(ren1);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPlaneSource> plane;
  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  ren1->AddActor(planeActor);

  iren->Initialize();
  renWin->SetSize(300, 150);
  renWin->Render();

  // Create widgets in different viewports. Note that SetCurrentRenderer()
  // should be set to prevent the automated detection of renderer which
  // screws things up with multiple renderers.
  svtkNew<svtkBorderWidget> borderWidget;
  borderWidget->SetInteractor(iren);
  borderWidget->SetCurrentRenderer(ren0);
  svtkNew<svtkBorderRepresentation> borderRep;
  borderRep->GetPositionCoordinate()->SetValue(0.1, 0.5);
  borderRep->GetPosition2Coordinate()->SetValue(0.4, 0.1);
  borderRep->SetShowBorderToOn();
  borderWidget->SetRepresentation(borderRep);
  borderWidget->On();

  svtkNew<svtkHandleWidget> handleWidget;
  handleWidget->SetCurrentRenderer(ren1);
  handleWidget->SetInteractor(iren);
  svtkNew<svtkPointHandleRepresentation2D> handleRep;
  handleRep->SetWorldPosition(plane->GetOrigin());
  handleWidget->SetRepresentation(handleRep);
  handleWidget->On();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  iren->Start();

  return EXIT_SUCCESS;
}
