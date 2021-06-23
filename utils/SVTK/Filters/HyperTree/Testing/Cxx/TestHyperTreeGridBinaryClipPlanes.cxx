/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinaryClipPlanes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, NexGen Analytics 2017

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridAxisClip.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"
#include "svtkHyperTreeGridToUnstructuredGrid.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkClipDataSet.h"
#include "svtkClipPolyData.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadric.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnstructuredGrid.h"

int TestHyperTreeGridBinaryClipPlanes(int argc, char* argv[])
{
  // Hyper tree grids with arrays of quadric values
  double xc = 1.;
  double yc = 1.;
  double zc = 0.;
  double q[10];
  q[0] = -1.;
  q[1] = -1.;
  q[2] = -1.;
  q[3] = 0.;
  q[4] = 0.;
  q[5] = 0.;
  q[6] = 2. * xc;
  q[7] = 2. * yc;
  q[8] = 2. * zc;
  q[9] = 1. - (xc * xc + yc * yc + zc * zc);
  svtkNew<svtkQuadric> quadric;
  quadric->SetCoefficients(q);
  svtkIdType res = 20;
  svtkNew<svtkHyperTreeGridSource> htg1;
  htg1->SetMaxDepth(0);
  htg1->SetDimensions(res + 1, 1, 1); // Dimension 1 suivant x GridCell res, 1, 1
  htg1->SetGridScale(2. / res, 0., 0.);
  htg1->SetBranchFactor(2);
  htg1->UseDescriptorOff();
  htg1->SetQuadric(quadric);
  svtkNew<svtkHyperTreeGridSource> htg2;
  htg2->SetMaxDepth(0);
  htg2->SetDimensions(res + 1, res + 1, 1); // Dimension 2 suivant xy plane GridCell res, res, 1
  htg2->SetGridScale(2. / res, 3. / res, 0.);
  htg2->SetBranchFactor(2);
  htg2->UseDescriptorOff();
  htg2->SetQuadric(quadric);
  svtkNew<svtkHyperTreeGridSource> htg3;
  htg3->SetMaxDepth(0);
  htg3->SetDimensions(res + 1, res + 1, res + 1); // GridCell res, res, res
  htg3->SetGridScale(2. / res, 3. / res, 4. / res);
  htg3->SetBranchFactor(2);
  htg3->UseDescriptorOff();
  htg3->SetQuadric(quadric);

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htg1->GetOutputPort());
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(htg2->GetOutputPort());

  // Conversion unstructured grid
  svtkNew<svtkHyperTreeGridToUnstructuredGrid> htg2ug;
  htg2ug->SetInputConnection(htg3->GetOutputPort());

  // Plane
  svtkNew<svtkPlane> plane;
  plane->SetOrigin(.4, .4, .4);
  plane->SetNormal(1., 1., 1.);

  // Planar clips
  svtkNew<svtkClipPolyData> clip1;
  clip1->SetInputConnection(geometry1->GetOutputPort());
  clip1->SetClipFunction(plane);
  clip1->Update();
  clip1->GetOutput()->GetCellData()->SetActiveScalars("Quadric");
  svtkNew<svtkClipPolyData> clip2;
  clip2->SetInputConnection(geometry2->GetOutputPort());
  clip2->SetClipFunction(plane);
  clip2->Update();
  clip2->GetOutput()->GetCellData()->SetActiveScalars("Quadric");
  svtkNew<svtkClipDataSet> clip3;
  clip3->SetInputConnection(htg2ug->GetOutputPort());
  clip3->SetClipFunction(plane);
  clip3->Update();
  clip3->GetOutput()->GetCellData()->SetActiveScalars("Quadric");

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(clip1->GetOutputPort());
  mapper1->SetScalarRange(clip1->GetOutput()->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(clip2->GetOutputPort());
  mapper2->SetScalarRange(clip2->GetOutput()->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkDataSetMapper> mapper3;
  mapper3->SetInputConnection(clip3->GetOutputPort());
  mapper3->SetScalarRange(clip3->GetOutput()->GetCellData()->GetScalars()->GetRange());

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->SetPosition(1.5, 0., 0.);
  actor1->GetProperty()->SetLineWidth(2);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->SetPosition(-2.5, 0., 0.);

  // Camera
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetViewUp(0., 1., 0.);
  camera->SetFocalPoint(.5, 1.5, 0.);
  camera->SetPosition(.5, 1.5, -7.);

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
  renWin->SetSize(600, 350);
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
