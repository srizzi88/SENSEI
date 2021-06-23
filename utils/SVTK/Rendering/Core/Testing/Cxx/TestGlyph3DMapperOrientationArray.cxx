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
#include "svtkConeSource.h"
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

int TestGlyph3DMapperOrientationArray(int argc, char* argv[])
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
  calc->AddScalarVariable("x", "Elevation");
  //  calc->AddCoordinateVectorVariable("p");
  calc->SetResultArrayName("orientation");
  calc->SetResultArrayType(SVTK_DOUBLE);
  calc->SetFunction("100*x*jHat");
  calc->Update();

  svtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->SetActiveScalars("Elevation");

  svtkActor* planeActor = svtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  planeActor->GetProperty()->SetRepresentationToWireframe();

  svtkConeSource* squad = svtkConeSource::New();
  squad->SetHeight(10.0);
  squad->SetRadius(1.0);
  squad->SetResolution(50);
  squad->SetDirection(0.0, 0.0, 1.0);

#ifdef USE_FILTER
  svtkGlyph3D* glypher = svtkGlyph3D::New();
  glypher->SetInputConnection(colors->GetOutputPort());
#else
  svtkGlyph3DMapper* glypher = svtkGlyph3DMapper::New();
  glypher->SetInputConnection(calc->GetOutputPort());
  glypher->SetOrientationArray("orientation");
  glypher->SetOrientationModeToRotation();
#endif
  glypher->SetScaleFactor(0.01);

  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();
  plane->Delete();
  calc->Delete();
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
