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
#include "svtkCamera.h"
#include "svtkElevationFilter.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSuperquadricSource.h"

// If USE_FILTER is defined, glyph3D->PolyDataMapper is used instead of
// Glyph3DMapper.
//#define USE_FILTER

#ifdef USE_FILTER
#include "svtkGlyph3D.h"
#else
#include "svtkGlyph3DMapper.h"
#endif

#include "svtkPlane.h"

int TestGlyph3DMapper(int argc, char* argv[])
{
  int res = 6;
  svtkPlaneSource* plane = svtkPlaneSource::New();
  plane->SetResolution(res, res);
  svtkElevationFilter* colors = svtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  plane->Delete();
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  svtkPolyDataMapper* planeMapper = svtkPolyDataMapper::New();
  planeMapper->SetInputConnection(colors->GetOutputPort());
  colors->Delete();

  svtkActor* planeActor = svtkActor::New();
  planeActor->SetMapper(planeMapper);
  planeMapper->Delete();
  planeActor->GetProperty()->SetRepresentationToWireframe();

  // create simple poly data so we can apply glyph
  svtkSuperquadricSource* squad = svtkSuperquadricSource::New();

#ifdef USE_FILTER
  svtkGlyph3D* glypher = svtkGlyph3D::New();
#else
  svtkGlyph3DMapper* glypher = svtkGlyph3DMapper::New();
#endif
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();

  // Useful code should you want to test clipping planes
  // with a glyph mapper, might should just uncomment
  // this and add a new valid image
  // svtkPlane *cplane = svtkPlane::New();
  // cplane->SetNormal(-0.5,0.5,0);
  // cplane->SetOrigin(0.2,0,0);
  // glypher->AddClippingPlane(cplane);

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
  win->AddRenderer(ren);
  ren->Delete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
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
