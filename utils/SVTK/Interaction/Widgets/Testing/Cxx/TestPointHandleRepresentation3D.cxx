/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkPointHandleRepresentation3D::PlaceWidget
// through svtkSeedWidget while changing the translation mode.
// If TranslationMode is set to False from outside, and PlaceWidget is called,
// the crosshair should be placed at the center of the bounds.

#include "svtkHandleWidget.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSeedRepresentation.h"
#include "svtkSeedWidget.h"
#include "svtkSmartPointer.h"

int TestPointHandleRepresentation3D(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> render = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(render);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create the widget and its representation
  svtkSmartPointer<svtkPointHandleRepresentation3D> handlePointRep3D =
    svtkSmartPointer<svtkPointHandleRepresentation3D>::New();
  handlePointRep3D->AllOn();
  handlePointRep3D->GetProperty()->SetColor(1., 0., 1.);

  svtkSmartPointer<svtkSeedRepresentation> seedRep = svtkSmartPointer<svtkSeedRepresentation>::New();
  seedRep->SetHandleRepresentation(handlePointRep3D);

  svtkSmartPointer<svtkSeedWidget> seedWidget = svtkSmartPointer<svtkSeedWidget>::New();

  seedWidget->SetRepresentation(seedRep);
  seedWidget->SetInteractor(iren);
  seedWidget->On();
  seedWidget->ProcessEventsOff();

  // Place two different points in different translation mode.
  double bounds[6] = { 0, 0.05, 0, 0.05, 0, 0.05 };
  double bounds2[6] = { -0.05, 0, -0.05, 0, -0.05, 0 };

  svtkHandleWidget* currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  svtkPointHandleRepresentation3D* handleRep =
    svtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->PlaceWidget(bounds);

  currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  handleRep = svtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->TranslationModeOff();
  handleRep->PlaceWidget(bounds2);

  // Add the actors to the renderer, set the background and size
  //
  render->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
