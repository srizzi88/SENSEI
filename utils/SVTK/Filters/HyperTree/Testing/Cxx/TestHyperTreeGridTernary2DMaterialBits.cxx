/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary2DMaterialBits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Joachim Pouderoux, Kitware 2013
// This test was revised by Philippe Pebay, NexGen Analytics 2017
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkBitArray.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkContourFilter.h"
#include "svtkDataSetMapper.h"
#include "svtkHyperTreeGridToDualGrid.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

int TestHyperTreeGridTernary2DMaterialBits(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1); // Dimension 2 in xy plane GridCell 2, 3, 1
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  const std::string descriptor =
    "RRRR.|" // Level 0 refinement
    "..R...... RRRRRRRRR R........ R........|..R...... ........R ......RRR ......RRR ..R..R..R "
    "RRRRRRRRR R..R..R.. ......... ......... ......... ......... .........|......... ......... "
    "......... ......... ......... ......... ......... ......... ........R ..R..R..R ......... "
    "......RRR ......R.. ......... RRRRRRRRR R..R..R.. ......... ......... ......... ......... "
    "......... ......... .........|......... ......... ......... ......... ......... ......... "
    "......... ......... ......... RRRRRRRRR ......... ......... ......... ......... ......... "
    "......... ......... ......... ......... .........|......... ......... ......... ......... "
    "......... ......... ......... ......... .........";
  const std::string materialMask = // Level 0 materials are not needed, visible cells are described
                                   // with LevelZeroMaterialIndex
    "111111111 111111111 111111111 111111111|111111111 000000001 000000111 011011111 001001001 "
    "111111111 100100100 001001001 111111111 111111111 111111111 001111111|111111111 001001001 "
    "111111111 111111111 111111111 111111111 111111111 111111111 001001111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111|111111111 111111111 111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111|111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111";
  svtkNew<svtkIdTypeArray> zero;
  zero->InsertNextValue(1);
  zero->InsertNextValue(2);
  zero->InsertNextValue(3);
  zero->InsertNextValue(4);
  zero->InsertNextValue(5);
  htGrid->UseMaskOn();
  htGrid->SetLevelZeroMaterialIndex(zero);
  svtkBitArray* desc = htGrid->ConvertDescriptorStringToBitArray(descriptor);
  svtkBitArray* mat = htGrid->ConvertMaskStringToBitArray(materialMask);
  htGrid->SetDescriptorBits(desc);
  desc->Delete();
  htGrid->SetMaskBits(mat);
  mat->Delete();

  // DualGrid
  svtkNew<svtkHyperTreeGridToDualGrid> dualFilter;
  dualFilter->SetInputConnection(htGrid->GetOutputPort());

  // Geometry
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  svtkPolyData* pd = geometry->GetPolyDataOutput();

  // Contour
  svtkNew<svtkContourFilter> contour;
  int nContours = 3;
  contour->SetNumberOfContours(nContours);
  contour->SetInputConnection(dualFilter->GetOutputPort());
  double resolution = (maxLevel - 1) / (nContours + 1.);
  double isovalue = resolution;
  for (int i = 0; i < nContours; ++i, isovalue += resolution)
  {
    contour->SetValue(i, isovalue);
  }

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(contour->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  svtkNew<svtkDataSetMapper> mapper4;
  mapper4->SetInputConnection(dualFilter->GetOutputPort());
  mapper4->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(.8, .4, .3);
  actor3->GetProperty()->SetLineWidth(3);
  svtkNew<svtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetRepresentationToWireframe();
  actor4->GetProperty()->SetColor(.0, .0, .0);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 6.);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(actor4);

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

  int retVal = svtkRegressionTestImageThreshold(renWin, 70);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
