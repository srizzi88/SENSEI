/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkArrayCalculator.h"
#include "svtkCamera.h"
#include "svtkElevationFilter.h"
#include "svtkInteractorStyleSwitch.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataReader.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkSuperquadricSource.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
//#define USE_FILTER

#ifdef USE_FILTER
#include "svtkGlyph3D.h"
#else
#include "svtkGlyph3DMapper.h"
#endif

int TestGlyph3DMapperMasking(int argc, char* argv[])
{
  int res = 30;
  svtkPlaneSource* plane = svtkPlaneSource::New();
  plane->SetResolution(res, res);
  svtkElevationFilter* colors = svtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  svtkPolyDataMapper* planeMapper = svtkPolyDataMapper::New();
  planeMapper->SetInputConnection(colors->GetOutputPort());
  colors->Delete();

  svtkArrayCalculator* calc = svtkArrayCalculator::New();
  calc->SetInputConnection(colors->GetOutputPort());
  calc->SetResultArrayName("mask");
  calc->SetResultArrayType(SVTK_BIT);
  calc->AddScalarArrayName("Elevation");
  calc->SetFunction("Elevation>0.2 & Elevation<0.4");
  calc->Update();

  svtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->GetArray("mask");
  svtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->SetActiveScalars("Elevation");

  svtkActor* planeActor = svtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  planeActor->GetProperty()->SetRepresentationToWireframe();

  // create simple poly data so we can apply glyph
  // svtkSuperquadricSource *squad=svtkSuperquadricSource::New();
  svtkSphereSource* squad = svtkSphereSource::New();
  squad->SetPhiResolution(45);
  squad->SetThetaResolution(45);

#ifdef USE_FILTER
  svtkGlyph3D* glypher = svtkGlyph3D::New();
  glypher->SetInputConnection(colors->GetOutputPort());
#else
  svtkGlyph3DMapper* glypher = svtkGlyph3DMapper::New();
  glypher->SetMasking(1);
  glypher->SetMaskArray("mask");
  glypher->SetInputConnection(calc->GetOutputPort());
  // glypher->SetInputConnection(colors->GetOutputPort());
  calc->Delete();
#endif
  //  glypher->SetScaleModeToDataScalingOn();
  glypher->SetScaleFactor(0.1);

  // glypher->SetInputConnection(plane->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();
  plane->Delete();

#ifdef USE_FILTER
  svtkPolyDataMapper* glyphMapper = svtkPolyDataMapper::New();
  glyphMapper->SetInputConnection(glypher->GetOutputPort());
#endif

  svtkActor* glyphActor = svtkActor::New();
#ifdef USE_FILTER
  glyphActor->SetMapper(glyphMapper);
  glyphMapper->Delete();
#else
  glyphActor->SetMapper(glypher);
#endif
  glypher->Delete();

  // Create the rendering stuff

  svtkRenderer* ren = svtkRenderer::New();
  svtkRenderWindow* win = svtkRenderWindow::New();
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  ren->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

  iren->SetRenderWindow(win);
  win->Delete();

  ren->AddActor(planeActor);
  planeActor->Delete();
  ren->AddActor(glyphActor);
  glyphActor->Delete();
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.5);

  win->Render();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
