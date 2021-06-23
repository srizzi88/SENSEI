/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernaryHyperbola.cxx

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

#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkContourFilter.h"
#include "svtkHyperTreeGridToDualGrid.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkQuadric.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkTextProperty.h"

int TestHyperTreeGridTernaryHyperbola(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(6);
  htGrid->SetDimensions(9, 13, 1); // GridCell 8, 12, 1
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->UseDescriptorOff();
  htGrid->UseMaskOff();
  svtkNew<svtkQuadric> quadric;
  quadric->SetCoefficients(1., -1., 0., 0., 0., 0., -12., 12., 0., 1.);
  htGrid->SetQuadric(quadric);

  // DualGrid
  svtkNew<svtkHyperTreeGridToDualGrid> dualFilter;
  dualFilter->SetInputConnection(htGrid->GetOutputPort());

  // Geometry
  svtkNew<svtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  svtkPolyData* pd = geometry->GetPolyDataOutput();
  pd->GetCellData()->SetActiveScalars("Quadric");

  // Contour
  svtkNew<svtkContourFilter> contour;
  contour->SetInputConnection(dualFilter->GetOutputPort());
  contour->SetNumberOfContours(0);
  contour->SetValue(0, 0);
  contour->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Quadric");
  //  Color transfer function
  svtkNew<svtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBSegment(-30., 0., 0., 1., 0., 0., 1., 1.);
  colorFunction->AddRGBSegment(SVTK_DBL_MIN, 1., 1., 0., 30., 1., 0., 0.);

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->UseLookupTableScalarRangeOn();
  mapper1->SetLookupTable(colorFunction);
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(contour->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  mapper3->SetRelativeCoincidentTopologyLineOffsetParameters(0.0, -8.0);

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(0., 0., 0.);
  actor3->GetProperty()->SetLineWidth(2);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 24.);

  // Scalar bar
  svtkNew<svtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(colorFunction);
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue(.65, .05);
  scalarBar->SetTitle("Quadric");
  scalarBar->SetWidth(0.15);
  scalarBar->SetHeight(0.4);
  scalarBar->SetTextPad(4);
  scalarBar->SetMaximumWidthInPixels(60);
  scalarBar->SetMaximumHeightInPixels(200);
  scalarBar->SetTextPositionToPrecedeScalarBar();
  scalarBar->GetTitleTextProperty()->SetColor(.4, .4, .4);
  scalarBar->GetLabelTextProperty()->SetColor(.4, .4, .4);
  scalarBar->SetDrawFrame(1);
  scalarBar->GetFrameProperty()->SetColor(.4, .4, .4);
  scalarBar->SetDrawBackground(1);
  scalarBar->GetBackgroundProperty()->SetColor(1., 1., 1.);

  // Renderer
  svtkNew<svtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(scalarBar);

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
