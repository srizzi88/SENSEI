/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the depth of field post-processing render pass.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"

#include "svtkDepthOfFieldPass.h"
#include "svtkRenderStepsPass.h"

#include "svtkCellArray.h"
#include "svtkTimerLog.h"

int TestDepthOfFieldPass(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkPolyDataMapper> mapper;
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());

  // create three dragons
  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(1.0, 0.0, 0.0);
    actor->GetProperty()->SetDiffuseColor(1.0, 0.8, 0.3);
    actor->GetProperty()->SetSpecular(0.0);
    actor->GetProperty()->SetDiffuse(0.5);
    actor->GetProperty()->SetAmbient(0.3);
    actor->SetPosition(-0.1, 0.0, -0.1);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
    actor->GetProperty()->SetDiffuseColor(0.2, 1.0, 0.8);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.2);
    actor->GetProperty()->SetDiffuse(0.9);
    actor->GetProperty()->SetAmbient(0.1);
    actor->GetProperty()->SetSpecularPower(10.0);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.5, 0.65, 1.0);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.7);
    actor->GetProperty()->SetDiffuse(0.4);
    actor->GetProperty()->SetSpecularPower(60.0);
    actor->SetPosition(0.1, 0.0, 0.1);
    renderer->AddActor(actor);
  }

  renderer->SetBackground(0.8, 0.8, 0.9);
  renderer->SetBackground2(1.0, 1.0, 1.0);
  renderer->GradientBackgroundOn();

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);

  // create the basic SVTK render steps
  svtkNew<svtkRenderStepsPass> basicPasses;

  // finally add the DOF passs
  svtkNew<svtkDepthOfFieldPass> dofp;
  dofp->SetDelegatePass(basicPasses);
  dofp->AutomaticFocalDistanceOff();
  // tell the renderer to use our render pass pipeline
  glrenderer->SetPass(dofp);

  renWin->SetSize(500, 500);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetFocalDisk(renderer->GetActiveCamera()->GetDistance() * 0.2);
  renWin->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 4;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0 / numRenders);
    renderer->GetActiveCamera()->Elevation(88.0 / numRenders);
    renWin->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " << numTris << endl;
  cerr << "triangles per second: " << numTris * (numRenders / elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.8);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
