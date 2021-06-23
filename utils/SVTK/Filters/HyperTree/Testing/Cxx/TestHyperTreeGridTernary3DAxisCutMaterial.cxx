/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DAxisCutMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2012
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridAxisCut.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridOutlineFilter.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkFilter.h"

int TestHyperTreeGridTernary3DAxisCutMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(5);
  htGrid->SetDimensions(4, 4, 3); // GridCell 3, 3, 2
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->UseMaskOn();
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
  htGrid->SetMask(
    "111 011 011 111 011 110|111111111111111111111111111 111111111111111111111111111 "
    "000000000100110111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "000110011100000100100010100|000001011011111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111001111111101111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111|000000000111100100111100100 000000000111001001111001001 "
    "000000111100100111111111111 000000111001001111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 111111111111111111111111111 "
    "111111111111111111111111111 111111111111111111111111111 "
    "110110110100111110111000000|111111111111111111111111111  11111111111111111111111111");

  // Outline
  svtkNew<svtkHyperTreeGridOutlineFilter> outline;
  outline->SetInputConnection(htGrid->GetOutputPort());

  // Axis cuts
  svtkNew<svtkHyperTreeGridAxisCut> axisCut1;
  axisCut1->SetInputConnection(htGrid->GetOutputPort());
  axisCut1->SetPlaneNormalAxis(0);
  axisCut1->SetPlanePosition(1.99);
  svtkNew<svtkHyperTreeGridAxisCut> axisCut2;
  axisCut2->SetInputConnection(htGrid->GetOutputPort());
  axisCut2->SetPlaneNormalAxis(2);
  axisCut2->SetPlanePosition(.35);

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(axisCut1->GetOutputPort());
  geometry1->Update();
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(axisCut2->GetOutputPort());
  geometry2->Update();
  svtkPolyData* pd = geometry2->GetPolyDataOutput();

  // Shrinks
  svtkNew<svtkShrinkFilter> shrink1;
  shrink1->SetInputConnection(geometry1->GetOutputPort());
  shrink1->SetShrinkFactor(.8);
  svtkNew<svtkShrinkFilter> shrink2;
  shrink2->SetInputConnection(geometry2->GetOutputPort());
  shrink2->SetShrinkFactor(.8);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(shrink1->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry1->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(outline->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  svtkNew<svtkDataSetMapper> mapper4;
  mapper4->SetInputConnection(shrink2->GetOutputPort());
  mapper4->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper5;
  mapper5->SetInputConnection(geometry2->GetOutputPort());
  mapper5->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(.1, .1, .1);
  actor3->GetProperty()->SetLineWidth(1);
  svtkNew<svtkActor> actor4;
  actor4->SetMapper(mapper4);
  svtkNew<svtkActor> actor5;
  actor5->SetMapper(mapper5);
  actor5->GetProperty()->SetRepresentationToWireframe();
  actor5->GetProperty()->SetColor(.7, .7, .7);

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
  renderer->AddActor(actor4);
  renderer->AddActor(actor5);

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

  // Compare bounds
  outline->Update();
  double outBd[6];
  outline->GetPolyDataOutput()->GetBounds(outBd);
  bool differenceDetected = false;
  for (int i = 0; i < 6; i++)
  {
    if ((bd[i] - outBd[i]) * (bd[i] - outBd[i]) > 0.0000001)
    {
      differenceDetected = true;
    }
  }

  if (differenceDetected)
  {
    std::cerr << "Error: REPORTED BOUNDS ARE INVALID" << std::endl;
    std::cerr << "htg: " << bd[0] << ", " << bd[1] << ", " << bd[2] << ", " << bd[3] << ", "
              << bd[4] << ", " << bd[5] << std::endl;
    std::cerr << "outline: " << outBd[0] << ", " << outBd[1] << ", " << outBd[2] << ", " << outBd[3]
              << ", " << outBd[4] << ", " << outBd[5] << std::endl;
    return 1; // Failed
  }

  int retVal = svtkRegressionTestImageThreshold(renWin, 25);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
