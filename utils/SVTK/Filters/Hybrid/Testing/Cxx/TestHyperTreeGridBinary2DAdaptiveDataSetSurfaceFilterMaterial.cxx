/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilterMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Rogeli Grima and Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkAdaptiveDataSetSurfaceFilter.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkHyperTreeGridSource.h"
#include "svtkNew.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilterMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3, 1
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->UseMaskOn();
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->SetMask("111111|0000 1111 1111 1111 1111|1111 0001 0111 0101 1011 1111 0111|1111 0111 "
                  "1111 1111 1111 1111|1111 1111 1111 1111|1111");

  // Data set surface
  svtkNew<svtkAdaptiveDataSetSurfaceFilter> surface;
  svtkNew<svtkRenderer> renderer;
  surface->SetRenderer(renderer);
  surface->SetInputConnection(htGrid->GetOutputPort());
  surface->Update();
  svtkPolyData* pd = surface->GetOutput();
  double* range = pd->GetCellData()->GetScalars()->GetRange();

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(surface->GetOutputPort());
  mapper1->SetScalarRange(range);
  svtkNew<svtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(surface->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  svtkNew<svtkCamera> camera;
  double point[3] = { pd->GetCenter()[0] - 0.75, pd->GetCenter()[1], pd->GetCenter()[2] };
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(point);
  point[2] += 10.0;
  camera->SetPosition(point);
  camera->ParallelProjectionOn();
  camera->SetParallelScale(1);

  // Renderer
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

  int retVal = svtkRegressionTestImageThreshold(renWin, 30);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
