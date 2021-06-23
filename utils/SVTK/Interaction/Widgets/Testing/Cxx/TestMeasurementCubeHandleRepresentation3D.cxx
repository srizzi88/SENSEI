/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMeasurementCubeHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkBillboardTextActor3D.h"
#include "svtkBoundedPlanePointPlacer.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCoordinate.h"
#include "svtkCubeSource.h"
#include "svtkCutter.h"
#include "svtkGlyph3D.h"
#include "svtkHandleWidget.h"
#include "svtkImplicitPlaneRepresentation.h"
#include "svtkImplicitPlaneWidget2.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLODActor.h"
#include "svtkMeasurementCubeHandleRepresentation3D.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPlane.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

int TestMeasurementCubeHandleRepresentation3D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  double bounds[6] = { -1., 1., -1., 1., -1., 1. };

  // Create the RenderWindow and Renderer
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // SVTK widgets consist of two parts: the widget part that handles event
  // processing, and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  svtkSmartPointer<svtkHandleWidget> handleWidget = svtkSmartPointer<svtkHandleWidget>::New();
  handleWidget->SetInteractor(iren);

  // Use a svtkMeasurementCubeHandleRepresentation3D to represent the handle widget
  svtkSmartPointer<svtkMeasurementCubeHandleRepresentation3D> unitCubeRep =
    svtkSmartPointer<svtkMeasurementCubeHandleRepresentation3D>::New();
  unitCubeRep->PlaceWidget(bounds);
  unitCubeRep->SetHandleSize(30);
  handleWidget->SetRepresentation(unitCubeRep);
  double p[3] = { 1., 0., 0. };
  unitCubeRep->SetWorldPosition(p);

  {
    // Create a sphere
    svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
    sphereSource->Update();

    // Create a mapper and actor
    svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
    actor->SetMapper(mapper);

    // Set the color of the sphere
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0); //(R,G,B)

    // Add the actor to the scene
    ren1->AddActor(actor);
  }

  // Set some defaults.
  //
  iren->Initialize();
  renWin->Render();
  handleWidget->EnabledOn();

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(400, 400);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
