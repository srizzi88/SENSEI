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
#include "svtkCellArray.h"
#include "svtkLightKit.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLTexture.h"
#include "svtkPLYReader.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShadowMapBakerPass.h"
#include "svtkTestUtilities.h"
#include "svtkTextureObject.h"
#include "svtkTimerLog.h"

//----------------------------------------------------------------------------
int TestShadowMapBakerPass(int argc, char* argv[])
{
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkPolyDataMapper> mapper;
  renderer->SetBackground(0.3, 0.4, 0.6);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  svtkNew<svtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());
  // mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  // actor->GetProperty()->SetRepresentationToWireframe();

  renderWindow->SetMultiSamples(0);

  svtkNew<svtkShadowMapBakerPass> bakerPass;

  // tell the renderer to use our render pass pipeline
  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(bakerPass);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "baking time: " << firstRender << endl;

  // get a shadow map
  svtkTextureObject* to = (*bakerPass->GetShadowMaps())[2];
  // by default the textures have depth comparison on
  // but for simple display we need to turn it off
  to->SetDepthTextureCompare(false);

  // now render this texture so we can see the depth map
  svtkNew<svtkActor> actor2;
  svtkNew<svtkPolyDataMapper> mapper2;
  svtkNew<svtkOpenGLTexture> texture;
  texture->SetTextureObject(to);
  actor2->SetTexture(texture);
  actor2->SetMapper(mapper2);

  svtkNew<svtkPlaneSource> plane;
  mapper2->SetInputConnection(plane->GetOutputPort());
  renderer->RemoveActor(actor);
  renderer->AddActor(actor2);
  glrenderer->SetPass(nullptr);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  bakerPass->ReleaseGraphicsResources(renderWindow);
  return !retVal;
}
