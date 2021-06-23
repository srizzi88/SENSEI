/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegendScaleActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests the terrain annotation capabilities in SVTK.
#include "svtkCamera.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLegendScaleActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestLegendScaleActor(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  ren1->GetActiveCamera()->ParallelProjectionOn();

  svtkInteractorStyleTrackballCamera* style = svtkInteractorStyleTrackballCamera::New();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create a test pipeline
  //
  svtkSphereSource* ss = svtkSphereSource::New();
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkActor* sph = svtkActor::New();
  sph->SetMapper(mapper);

  // Create the actor
  svtkLegendScaleActor* actor = svtkLegendScaleActor::New();
  actor->TopAxisVisibilityOn();

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(sph);
  ren1->AddViewProp(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  ss->Delete();
  mapper->Delete();
  sph->Delete();
  actor->Delete();
  style->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();

  return !retVal;
}
