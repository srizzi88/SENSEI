/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DAxisClipBox.cxx

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
#include "svtkHyperTreeGridAxisClip.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCubeSource.h"
#include "svtkDataSetMapper.h"
#include "svtkExtractEdges.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTubeFilter.h"

int TestHyperTreeGridTernary3DAxisClipBox(int argc, char* argv[])
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

  // Axis clip
  svtkNew<svtkHyperTreeGridAxisClip> clip;
  clip->SetInputConnection(htGrid->GetOutputPort());
  clip->SetClipTypeToBox();
  double x0 = 1.6;
  double x1 = 2.2;
  double y0 = 1.4;
  double y1 = 2.5;
  double z0 = -.5;
  double z1 = 1.9;
  clip->SetBounds(x0, x1, y0, y1, z0, z1);

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htGrid->GetOutputPort());
  geometry1->Update();
  svtkPolyData* pd = geometry1->GetPolyDataOutput();
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(clip->GetOutputPort());

  // Box
  svtkNew<svtkCubeSource> cube;
  cube->SetBounds(x0, x1, y0, y1, z0, z1);

  // Edges
  svtkNew<svtkExtractEdges> edges;
  edges->SetInputConnection(cube->GetOutputPort());

  // Tubes
  svtkNew<svtkTubeFilter> tubes;
  tubes->SetInputConnection(edges->GetOutputPort());
  tubes->SetRadius(0.015);
  tubes->SetNumberOfSides(50);
  tubes->UseDefaultNormalOn();
  tubes->SetDefaultNormal(.577, .577, .577);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(geometry2->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry1->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(tubes->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(.3, .3, .3);

  // Camera
  svtkHyperTreeGrid* ht = htGrid->GetHyperTreeGridOutput();
  double bd[6];
  ht->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(ht->GetCenter());
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

  int retVal = svtkRegressionTestImageThreshold(renWin, 25);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
