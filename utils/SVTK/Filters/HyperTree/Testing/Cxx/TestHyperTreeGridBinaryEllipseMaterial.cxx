/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinaryEllipseMaterial.cxx

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
#include "svtkNew.h"
#include "svtkPolyData.h"
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

int TestHyperTreeGridBinaryEllipseMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(8);
  htGrid->SetDimensions(17, 25, 1); // Dimension 2 in xy plane GridCell 16, 24, 1
  htGrid->SetGridScale(.5, .25, .7);
  htGrid->SetBranchFactor(2);
  htGrid->UseDescriptorOff();
  htGrid->UseMaskOn();
  svtkNew<svtkQuadric> quadric;
  quadric->SetCoefficients(-4., -9., 0., 0., 0., 0., 32., 54., 0., -109.);
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
  int nContours = 6;
  contour->SetNumberOfContours(nContours);
  double isovalue = -90.;
  for (int i = 0; i < nContours; ++i, isovalue += 16.)
  {
    contour->SetValue(i, isovalue);
  }
  contour->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Quadric");
  //  Color transfer function
  svtkNew<svtkColorTransferFunction> colorFunction;
  colorFunction->AddHSVSegment(-90., .667, 1., 1., 0., 0., 1., 1.);

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

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.2, .9, .2);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 15.5);

  // Scalar bar
  svtkNew<svtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(colorFunction);
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue(.45, .3);
  scalarBar->SetTitle("Quadric");
  scalarBar->SetNumberOfLabels(4);
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

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
