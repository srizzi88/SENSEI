/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DPlaneCutterDual.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridPlaneCutter.h"
#include "svtkHyperTreeGridSource.h"
#include "svtkHyperTreeGridToUnstructuredGrid.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkFilter.h"
#include "svtkUnstructuredGrid.h"

int TestHyperTreeGridTernary3DPlaneCutterDual(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(5);
  htGrid->SetDimensions(4, 4, 3); // GridCell 3, 3, 2
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->SetDescriptor(
    "RRR .R. .RR ..R ..R .R.|R.......................... ........................... "
    "........................... .............R............. ....RR.RR........R......... "
    ".....RRRR.....R.RR......... ........................... ........................... "
    "...........................|........................... ........................... "
    "........................... ...RR.RR.......RR.......... ........................... "
    "RR......................... ........................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... "
    "............RRR............|........................... ........................... "
    ".......RR.................. ........................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... "
    "...........................|........................... ...........................");

  // Hyper tree grid to unstructured grid filter
  svtkNew<svtkHyperTreeGridToUnstructuredGrid> htg2ug;
  htg2ug->SetInputConnection(htGrid->GetOutputPort());
  htg2ug->Update();
  svtkUnstructuredGrid* ug = htg2ug->GetUnstructuredGridOutput();
  double* range = ug->GetCellData()->GetScalars()->GetRange();

  // Plane cutters
  svtkNew<svtkHyperTreeGridPlaneCutter> cut1;
  cut1->SetInputConnection(htGrid->GetOutputPort());
  cut1->SetPlane(1., -.2, .2, 3.);
  cut1->DualOn();
  svtkNew<svtkHyperTreeGridPlaneCutter> cut2;
  cut2->SetInputConnection(htGrid->GetOutputPort());
  cut2->SetPlane(-.2, -.6, 1., .05);
  cut2->DualOn();

  // Geometry
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();

  // Shrinks
  svtkNew<svtkShrinkFilter> shrink1;
  shrink1->SetInputConnection(cut1->GetOutputPort());
  shrink1->SetShrinkFactor(.95);
  svtkNew<svtkShrinkFilter> shrink2;
  shrink2->SetInputConnection(cut2->GetOutputPort());
  shrink2->SetShrinkFactor(.95);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(shrink1->GetOutputPort());
  mapper1->SetScalarRange(range);
  svtkNew<svtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(shrink2->GetOutputPort());
  mapper2->SetScalarRange(range);
  svtkNew<svtkDataSetMapper> mapper3;
  mapper3->SetInputConnection(htg2ug->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  ug->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(ug->GetCenter());
  camera->SetPosition(-.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5]);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);

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

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
