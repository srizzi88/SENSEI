/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinary2DVectorAxisReflectionYCenter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, 2016
// This test was modified by Philippe Pebay, NexGen Analytics 2017
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridAxisReflection.h"
#include "svtkHyperTreeGridCellCenters.h"
#include "svtkHyperTreeGridGeometry.h"
#include "svtkHyperTreeGridSource.h"

#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkGlyph2D.h"
#include "svtkGlyphSource2D.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestHyperTreeGridBinary2DVectorAxisReflectionYCenter(int argc, char* argv[])
{
  // Hyper tree grid
  svtkNew<svtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(6);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->GenerateInterfaceFieldsOn();
  htGrid->Update();
  svtkHyperTreeGrid* H = svtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  H->SetHasInterface(1);
  char normalsName[] = "Normals";
  H->SetInterfaceNormalsName(normalsName);
  char interceptsName[] = "Intercepts";
  H->SetInterfaceInterceptsName(interceptsName);

  // Axis reflection
  svtkNew<svtkHyperTreeGridAxisReflection> reflection;
  reflection->SetInputConnection(htGrid->GetOutputPort());
  reflection->SetPlaneToY();
  reflection->SetCenter(1.5);

  // Cell centers
  svtkNew<svtkHyperTreeGridCellCenters> centers;
  centers->SetInputConnection(reflection->GetOutputPort());
  centers->VertexCellsOn();

  // 2D glyph source
  svtkNew<svtkGlyphSource2D> glyph;
  glyph->SetGlyphTypeToArrow();
  glyph->SetScale(.8);
  glyph->FilledOff();

  // Glyphs
  svtkNew<svtkGlyph2D> glypher;
  glypher->SetInputConnection(centers->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetScaleModeToScaleByVector();
  glypher->OrientOn();

  // Geometries
  svtkNew<svtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(reflection->GetOutputPort());
  geometry1->Update();
  svtkPolyData* pd = geometry1->GetPolyDataOutput();
  svtkNew<svtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(reflection->GetOutputPort());

  // Mappers
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry1->GetOutputPort());
  mapper1->ScalarVisibilityOff();
  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry2->GetOutputPort());
  mapper2->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  svtkNew<svtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(glypher->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetRepresentationToWireframe();
  actor1->GetProperty()->SetColor(.7, .7, .7);
  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);
  svtkNew<svtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(0., 0., 0.);
  actor3->GetProperty()->SetLineWidth(1);

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
