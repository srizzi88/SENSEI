/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridToDualGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// This test verifies that SVTK can obtain the dual grid representation
// for a HyperTreeGrid.

#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkHyperTreeGridToDualGrid.h"
#include "svtkHyperTreeGridToUnstructuredGrid.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnstructuredGrid.h"

#include "svtkDataSetWriter.h"

int TestHyperTreeGridToDualGrid(int argc, char* argv[])
{
  // Writer for debug
  // svtkNew<svtkDataSetWriter> writer;

  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");

  // Geometry
  svtkNew<svtkHyperTreeGridToDualGrid> dualfilter;
  dualfilter->SetInputConnection(htGrid->GetOutputPort());
  dualfilter->Update();
  svtkUnstructuredGrid* dual = svtkUnstructuredGrid::SafeDownCast(dualfilter->GetOutput());
  // dual->PrintSelf(cerr, svtkIndent(0));
  // writer->SetFileName("fooDual.svtk");
  // writer->SetInputData(dual);
  // writer->Write();

  svtkNew<svtkHyperTreeGridToUnstructuredGrid> gfilter;
  gfilter->SetInputConnection(htGrid->GetOutputPort());
  gfilter->Update();
  // svtkUnstructuredGrid *primal = svtkUnstructuredGrid::SafeDownCast(gfilter->GetOutput());
  // writer->SetFileName("fooPrimal.svtk");
  // writer->SetInputData(primal);
  // writer->Write();

  svtkNew<svtkHyperTreeGridGeometry> sfilter;
  sfilter->SetInputConnection(htGrid->GetOutputPort());
  sfilter->Update();
  // svtkPolyData *skin = svtkPolyData::SafeDownCast(sfilter->GetOutput());
  // writer->SetFileName("fooSkin.svtk");
  // writer->SetInputData(skin);
  // writer->Write();

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(dualfilter->GetOutputPort());
  // mapper1->SetScalarRange( dual->GetCellData()->GetScalars()->GetRange() ); //no cell data yet
  svtkNew<svtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(dualfilter->GetOutputPort());
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
  dual->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(dual->GetCenter());
  camera->SetPosition(.5 * (bd[0] + bd[1]), .5 * (bd[2] + bd[3]), 6.);

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

  int retVal = svtkRegressionTestImageThreshold(renWin, 2);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
