/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DContour.cxx

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

#include "svtkHyperTreeGridContour.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestHyperTreeGridTernary3DContour(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  int maxLevel = 5;
  htGrid->SetMaxDepth(maxLevel);
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

  // Contour
  svtkNew<svtkHyperTreeGridContour> contour;
  contour->SetInputConnection(htGrid->GetOutputPort());
  int nContours = 4;
  contour->SetNumberOfContours(nContours);
  contour->SetInputConnection(htGrid->GetOutputPort());
  double resolution = (maxLevel - 1) / (nContours + 1.);
  double isovalue = resolution;
  for (int i = 0; i < nContours; ++i, isovalue += resolution)
  {
    contour->SetValue(i, isovalue);
  }

  // Geometry
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  svtkPolyData* pd = geometry->GetPolyDataOutput();

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(contour->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(contour->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(geometry->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.3, .3, .3);
  actor2->GetProperty()->SetLineWidth(1);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
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

  int retVal = svtkRegressionTestImageThreshold(renWin, 60);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
