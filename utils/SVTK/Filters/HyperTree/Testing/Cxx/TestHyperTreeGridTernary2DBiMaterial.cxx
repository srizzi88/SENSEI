/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary2DBiMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2013
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkFilter.h"

int TestHyperTreeGridTernary2DBiMaterial(int argc, char* argv[])
{
  // Hyper tree grids
  svtkNew<svtkHyperTreeGridSource> htGrid1;
  htGrid1->SetMaxDepth(3);
  htGrid1->SetOrigin(0., 0., 0.);
  htGrid1->SetDimensions(3, 2, 1); // Dimension 2 in xy plane GridCell 2, 1, 1
  htGrid1->SetGridScale(1., 1., 1.);
  htGrid1->SetBranchFactor(3);
  htGrid1->UseMaskOn();
  htGrid1->SetDescriptor(".R|.R..R..R.|......... ......... .........");
  htGrid1->SetMask("11|110110110|110110110 110110110 110110110");
  svtkNew<svtkHyperTreeGridSource> htGrid2;
  htGrid2->SetMaxDepth(3);
  htGrid2->SetOrigin(1., 0., 0.);
  htGrid2->SetDimensions(3, 2, 1); // Dimension 2 in xy plane GridCell 3, 2, 1
  htGrid2->SetGridScale(1., 1., 1.);
  htGrid2->SetBranchFactor(3);
  htGrid2->UseMaskOn();
  htGrid2->SetDescriptor("R.|.R..R..R.|......... ......... .........");
  htGrid2->SetMask("11|011011011|011011011 011011011 011011011");

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htGrid1->GetOutputPort());
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(htGrid2->GetOutputPort());

  // Shrinks
  svtkNew<svtkShrinkFilter> shrink1;
  shrink1->SetInputConnection(geometry1->GetOutputPort());
  shrink1->SetShrinkFactor(.8);

  // Mappers
  geometry1->Update();
  svtkPolyData* pd1 = geometry1->GetPolyDataOutput();
  geometry2->Update();
  svtkPolyData* pd2 = geometry2->GetPolyDataOutput();
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(shrink1->GetOutputPort());
  mapper1->SetScalarRange(pd1->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry2->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(0., 0., 0.);
  actor2->GetProperty()->SetLineWidth(2);

  // Camera
  double bd1[6];
  pd1->GetBounds(bd1);
  double bd2[6];
  pd2->GetBounds(bd2);
  double bd[4];
  for (int i = 0; i < 3; ++i)
  {
    bd[i] = bd1[i] < bd2[i] ? bd1[i] : bd2[i];
    ++i;
    bd[i] = bd1[i] > bd2[i] ? bd1[i] : bd2[i];
  }
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  double xc = .5 * (bd[0] + bd[1]);
  double yc = .5 * (bd[2] + bd[3]);
  camera->SetFocalPoint(xc, yc, 0.);
  camera->SetPosition(xc, yc, 2.);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Render window
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(600, 200);
  renWin->SetMultiSamples(0);

  // Interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 20);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
