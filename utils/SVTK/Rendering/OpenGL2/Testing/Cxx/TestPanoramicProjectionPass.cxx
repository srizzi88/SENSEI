/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPanoramicProjectionPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the tone mapping post-processing render pass.
// It renders an opaque actor with a lot of lights.

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCameraPass.h"
#include "svtkCullerCollection.h"
#include "svtkLight.h"
#include "svtkLightsPass.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPanoramicProjectionPass.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSphereSource.h"

int TestPanoramicProjectionPass(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(1.0);

  svtkNew<svtkRenderer> renderer;
  renderer->GetCullers()->RemoveAllItems();
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->AutomaticLightCreationOff();

  svtkNew<svtkLight> light;
  light->SetPosition(0.0, 10.0, 0.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetLightTypeToSceneLight();
  renderer->AddLight(light);

  // custom passes
  svtkNew<svtkCameraPass> cameraP;
  svtkNew<svtkSequencePass> seq;
  svtkNew<svtkOpaquePass> opaque;
  svtkNew<svtkLightsPass> lights;

  svtkNew<svtkRenderPassCollection> passes;
  passes->AddItem(lights);
  passes->AddItem(opaque);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  svtkNew<svtkPanoramicProjectionPass> projectionP;
  projectionP->SetProjectionTypeToAzimuthal();
  projectionP->SetAngle(360.0);
  projectionP->SetDelegatePass(cameraP);

  svtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(projectionP);

  renWin->AddRenderer(renderer);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 4; i++)
  {
    double f = (i & 1) ? -2.0 : 2.0;
    double x = (i & 2) ? 1.0 : 0.0;
    double c[3] = { static_cast<double>((i + 1) & 1), static_cast<double>(((i + 1) >> 1) & 1),
      static_cast<double>(((i + 1) >> 2) & 1) };
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->SetPosition(f * x, 0.0, f * (1.0 - x));
    actor->GetProperty()->SetColor(c);
    renderer->AddActor(actor);
  }

  svtkNew<svtkCamera> camera;
  camera->SetPosition(0.0, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, 1.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  renderer->SetActiveCamera(camera);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
