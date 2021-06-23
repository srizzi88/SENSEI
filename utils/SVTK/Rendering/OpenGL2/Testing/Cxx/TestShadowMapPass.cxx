/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// test baking shadow maps
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkCellArray.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLTexture.h"
#include "svtkPLYReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderStepsPass.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSequencePass.h"
#include "svtkShadowMapBakerPass.h"
#include "svtkShadowMapPass.h"
#include "svtkTestUtilities.h"
#include "svtkTimerLog.h"

//----------------------------------------------------------------------------
int TestShadowMapPass(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkLight> light1;
  light1->SetFocalPoint(0, 0, 0);
  light1->SetPosition(0, 1, 0.2);
  light1->SetColor(0.95, 0.97, 1.0);
  light1->SetIntensity(0.8);
  renderer->AddLight(light1);

  svtkNew<svtkLight> light2;
  light2->SetFocalPoint(0, 0, 0);
  light2->SetPosition(1.0, 1.0, 1.0);
  light2->SetColor(1.0, 0.8, 0.7);
  light2->SetIntensity(0.3);
  renderer->AddLight(light2);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.135, 0.2225, 0.3);
  actor->GetProperty()->SetDiffuseColor(0.54, 0.89, 0.63);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.51);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.7);
  actor->GetProperty()->SetSpecularPower(30.0);
  actor->GetProperty()->SetOpacity(1.0);
  renderer->AddActor(actor);
  // actor->GetProperty()->SetRepresentationToWireframe();

  // add a plane
  svtkNew<svtkPlaneSource> plane;
  const double* plybounds = mapper->GetBounds();
  plane->SetOrigin(-0.2, plybounds[2], -0.2);
  plane->SetPoint1(0.2, plybounds[2], -0.2);
  plane->SetPoint2(-0.2, plybounds[2], 0.2);
  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  renderer->AddActor(planeActor);

  renderWindow->SetMultiSamples(0);

  svtkNew<svtkShadowMapPass> shadows;

  svtkNew<svtkSequencePass> seq;
  svtkNew<svtkRenderPassCollection> passes;
  passes->AddItem(shadows->GetShadowMapBakerPass());
  passes->AddItem(shadows);
  seq->SetPasses(passes);

  svtkNew<svtkCameraPass> cameraP;
  cameraP->SetDelegatePass(seq);

  // tell the renderer to use our render pass pipeline
  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(cameraP);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 8;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0 / numRenders);
    renderer->GetActiveCamera()->Elevation(80.0 / numRenders);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " << numTris << endl;
  cerr << "triangles per second: " << numTris * (numRenders / elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(-0.2, 0.2, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.5);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
