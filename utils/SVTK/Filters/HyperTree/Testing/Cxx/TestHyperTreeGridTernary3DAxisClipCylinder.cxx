/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DAxisClipCylinder.cxx

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
#include "svtkContourFilter.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadric.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSampleFunction.h"

int TestHyperTreeGridTernary3DAxisClipCylinder(int argc, char* argv[])
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
  clip->SetClipTypeToQuadric();
  double r = .7;
  double xc = 2.33;
  double yc = 2.;
  double q[10];
  q[0] = 1.;
  q[1] = 1.;
  q[2] = 0.;
  q[3] = 0.;
  q[4] = 0.;
  q[5] = 0.;
  q[6] = -2. * xc;
  q[7] = -2. * yc;
  q[8] = 0.;
  q[9] = xc * xc + yc * yc - r * r;
  clip->SetQuadricCoefficients(q);

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htGrid->GetOutputPort());
  geometry1->Update();
  svtkPolyData* pd = geometry1->GetPolyDataOutput();
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(clip->GetOutputPort());

  // Cylinder
  svtkHyperTreeGrid* ht = htGrid->GetHyperTreeGridOutput();
  double bd[6];
  ht->GetBounds(bd);
  double x0 = bd[0];
  double x1 = bd[1];
  double y0 = bd[2];
  double y1 = bd[3];
  double z0 = bd[4] - .2;
  double z1 = bd[5] + .2;
  svtkNew<svtkSampleFunction> sample;
  sample->SetSampleDimensions(50, 50, 2);
  sample->SetModelBounds(x0, x1, y0, y1, z0, z1);
  sample->SetImplicitFunction(clip->GetQuadric());
  sample->ComputeNormalsOn();
  svtkNew<svtkContourFilter> cylinder;
  cylinder->SetInputConnection(sample->GetOutputPort());
  cylinder->GenerateValues(1, 0., 0.);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(geometry2->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry1->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(cylinder->GetOutputPort());
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

  // Camera
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

  int retVal = svtkRegressionTestImageThreshold(renWin, 80);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
