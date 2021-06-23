/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegendBoxActor2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the terrain annotation capabilities in SVTK.
#include "svtkCamera.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLegendBoxActor.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestLegendBoxActor2(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  ren1->GetActiveCamera()->ParallelProjectionOn();

  svtkSmartPointer<svtkInteractorStyleTrackballCamera> style =
    svtkSmartPointer<svtkInteractorStyleTrackballCamera>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Colors.
  double textColor[5][3] = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 },
    { 1.0, 0.5, 0.5 }, { 0.5, 1.0, 0.5 } };

  double backgroundColor[3] = { 0.8, 0.5, 0.0 };

  const char* text[5] = { "Text1", "Text2", "Text3", "Text4", "Text5" };

  // Create the actor
  svtkSmartPointer<svtkLegendBoxActor> actor = svtkSmartPointer<svtkLegendBoxActor>::New();
  actor->SetNumberOfEntries(5);
  actor->SetUseBackground(1);
  actor->SetBackgroundColor(backgroundColor);
  actor->SetBackgroundOpacity(1.0);

  actor->GetPositionCoordinate()->SetCoordinateSystemToView();
  actor->GetPositionCoordinate()->SetValue(-0.7, -0.8);

  actor->GetPosition2Coordinate()->SetCoordinateSystemToView();
  actor->GetPosition2Coordinate()->SetValue(0.7, 0.8);

  // Create a test pipeline
  //
  for (int i = 0; i < 5; ++i)
  {
    svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
    sphere->SetRadius(static_cast<double>(10 * (i + 1)));
    sphere->Update();
    actor->SetEntry(i, sphere->GetOutput(), text[i], textColor[i]);
  }

  // Add the actors to the renderer, set the background and size
  ren1->AddViewProp(actor);
  ren1->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(350, 350);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
