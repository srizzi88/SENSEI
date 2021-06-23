/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernarySphereMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2012
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadric.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

int TestHyperTreeGridTernarySphereMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(4);
  htGrid->SetDimensions(6, 6, 7); // GridCell 5, 5, 6
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->UseDescriptorOff();
  htGrid->UseMaskOn();
  svtkNew<svtkQuadric> quadric;
  quadric->SetCoefficients(1., 1., 1., 0, 0., 0., 0.0, 0., 0., -25.);
  htGrid->SetQuadric(quadric);
  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  cerr << "Creation time : " << timer->GetElapsedTime() << endl;
  timer->StartTimer();
  svtkNew<svtkHyperTreeGrid> htgCopy;
  htgCopy->ShallowCopy(htGrid->GetOutput());
  timer->StopTimer();
  cerr << "Copy time : " << timer->GetElapsedTime() << endl;
  // Geometry
  timer->StartTimer();
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputData(htgCopy);
  geometry->Update();
  svtkPolyData* pd = geometry->GetPolyDataOutput();
  timer->StopTimer();
  cerr << "Geometry time : " << timer->GetElapsedTime() << endl;

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(-.7 * bd[1], .9 * bd[3], -2.5 * bd[5]);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Render window
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 110);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
