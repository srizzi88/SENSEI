/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2013
// This test was modified by Philippe Pebay, NexGen Analytics 2017
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridSource.h"
#include "svtkHyperTreeGridToUnstructuredGrid.h"

#include "svtkCamera.h"
#include "svtkClipDataSet.h"
#include "svtkDataSetMapper.h"
#include "svtkHyperTreeGridToDualGrid.h"
#include "svtkNew.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkFilter.h"
#include "svtkUnstructuredGrid.h"

int TestHyperTreeGridTernary3DClip(int argc, char* argv[])
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

  // DualGrid
  svtkNew<svtkHyperTreeGridToDualGrid> dualFilter;
  dualFilter->SetInputConnection(htGrid->GetOutputPort());

  // To unstructured grid
  svtkNew<svtkHyperTreeGridToUnstructuredGrid> htg2ug;
  htg2ug->SetInputConnection(htGrid->GetOutputPort());

  // Clip
  svtkNew<svtkPlane> plane;
  plane->SetOrigin(0., .5, .4);
  plane->SetNormal(-.2, -.6, 1.);
  svtkNew<svtkClipDataSet> clip;
  clip->SetInputConnection(dualFilter->GetOutputPort());
  clip->SetClipFunction(plane);
  clip->Update();

  // Shrink
  svtkNew<svtkShrinkFilter> shrink;
  shrink->SetInputConnection(clip->GetOutputPort());
  shrink->SetShrinkFactor(.8);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(htg2ug->GetOutputPort());
  mapper1->ScalarVisibilityOff();
  svtkNew<svtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(shrink->GetOutputPort());
  mapper2->SetScalarRange(clip->GetOutput()->GetPointData()->GetScalars()->GetRange());

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetRepresentationToWireframe();
  actor1->GetProperty()->SetColor(.8, .8, .8);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);

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

  int retVal = svtkRegressionTestImageThreshold(renWin, 40);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
