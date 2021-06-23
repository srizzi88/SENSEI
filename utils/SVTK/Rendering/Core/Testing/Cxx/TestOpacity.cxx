/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpacity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering translucent materials with depth peeling
// technique.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCubeSource.h"
#include "svtkGlyph3D.h"
#include "svtkImageData.h"
#include "svtkImageGridSource.h"
#include "svtkLookupTable.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

// if not defined, we use spherical glyphs (slower) instead of cubic
// glyphs (faster)
// #define SVTK_TEST_OPACITY_CUBE

int TestOpacity(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  renderer->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  // We create a bunch of translucent spheres with an opaque plane in
  // the middle
  // we create a uniform grid and glyph it with a spherical shape.

  // Create the glyph source
  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetRadius(1);
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetThetaResolution(10);
  sphere->SetPhiResolution(10);
  sphere->SetLatLongTessellation(0);

  svtkCubeSource* cube = svtkCubeSource::New();
  cube->SetXLength(1.0);
  cube->SetYLength(1.0);
  cube->SetZLength(1.0);
  cube->SetCenter(0.0, 0.0, 0.0);

  svtkImageGridSource* grid = svtkImageGridSource::New();
  grid->SetGridSpacing(1, 1, 1);
  grid->SetGridOrigin(0, 0, 0);
  grid->SetLineValue(1.0); // white
  grid->SetFillValue(0.5); // gray
  grid->SetDataScalarTypeToUnsignedChar();
  grid->SetDataExtent(0, 10, 0, 10, 0, 10);
  grid->SetDataSpacing(0.1, 0.1, 0.1);
  grid->SetDataOrigin(0.0, 0.0, 0.0);
  grid->Update(); // to get the range

  double range[2];
  grid->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  svtkGlyph3D* glyph = svtkGlyph3D::New();
  glyph->SetInputConnection(0, grid->GetOutputPort(0));
  grid->Delete();
#ifdef SVTK_TEST_OPACITY_CUBE
  glyph->SetSourceConnection(cube->GetOutputPort(0));
#else
  glyph->SetSourceConnection(sphere->GetOutputPort(0));
#endif
  sphere->Delete();
  cube->Delete();
  glyph->SetScaling(1); // on
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetColorModeToColorByScale();
  glyph->SetScaleFactor(0.05);
  glyph->SetRange(range);
  glyph->SetOrient(0);
  glyph->SetClamping(0);
  glyph->SetVectorModeToUseVector();
  glyph->SetIndexModeToOff();
  glyph->SetGeneratePointIds(0);

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(glyph->GetOutputPort(0));
  glyph->Delete();

  // This creates a blue to red lut.
  svtkLookupTable* lut = svtkLookupTable::New();
  lut->SetHueRange(0.667, 0.0);
  mapper->SetLookupTable(lut);
  lut->Delete();
  mapper->SetScalarRange(range);

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();
  renderer->AddActor(actor);
  actor->Delete();

  svtkProperty* property = svtkProperty::New();
  property->SetOpacity(0.2);
  property->SetColor(0.0, 1.0, 0.0);
  actor->SetProperty(property);
  property->Delete();

  svtkPlaneSource* plane = svtkPlaneSource::New();
  plane->SetCenter(0.5, 0.5, 0.5);

  svtkPolyDataMapper* planeMapper = svtkPolyDataMapper::New();
  planeMapper->SetInputConnection(0, plane->GetOutputPort(0));
  plane->Delete();

  svtkActor* planeActor = svtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  renderer->AddActor(planeActor);

  svtkProperty* planeProperty = svtkProperty::New();
  planeProperty->SetOpacity(1.0);
  planeProperty->SetColor(1.0, 0.0, 0.0);
  planeActor->SetProperty(planeProperty);
  planeProperty->Delete();
  planeProperty->SetBackfaceCulling(0);
  planeProperty->SetFrontfaceCulling(0);
  planeActor->Delete();

  renderer->SetUseDepthPeeling(1);
  // reasonable depth peeling settings
  // no more than 50 layers of transluceny
  renderer->SetMaximumNumberOfPeels(50);
  // stop when less than 2 in 1000 pixels changes
  renderer->SetOcclusionRatio(0.002);

  property->SetBackfaceCulling(1);
  property->SetFrontfaceCulling(0);

  // Standard testing code.
  renderer->SetBackground(0.0, 0.5, 0.0);
  renWin->SetSize(300, 300);
  renWin->Render();

  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  iren->Delete();

  return !retVal;
}
