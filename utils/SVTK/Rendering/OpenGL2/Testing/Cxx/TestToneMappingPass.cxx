/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestToneMappingPass.cxx

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
#include "svtkLight.h"
#include "svtkLightsPass.h"
#include "svtkOpaquePass.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSphereSource.h"
#include "svtkToneMappingPass.h"

int TestToneMappingPass(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 800);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);

  double y = 0.0;
  for (int i = 0; i < 8; i++)
  {
    svtkNew<svtkRenderer> renderer;

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

    svtkNew<svtkToneMappingPass> toneMappingP;
    switch (i)
    {
      case 0:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::Clamp);
        break;
      case 1:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::Reinhard);
        break;
      case 2:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::Exponential);
        toneMappingP->SetExposure(1.0);
        break;
      case 3:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::Exponential);
        toneMappingP->SetExposure(2.0);
        break;
      case 4:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicUncharted2Presets();
        break;
      case 5:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicDefaultPresets();
        break;
      case 6:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::GenericFilmic);
        toneMappingP->SetUseACES(false);
        break;
      case 7:
        toneMappingP->SetToneMappingType(svtkToneMappingPass::GenericFilmic);
        toneMappingP->SetGenericFilmicUncharted2Presets();
        toneMappingP->SetUseACES(false);
        break;
    }
    toneMappingP->SetDelegatePass(cameraP);

    svtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(toneMappingP);

    double x = 0.5 * (i & 1);
    if (i)
    {
      y += 1 / 4.f * !(i & 1);
    }

    renderer->SetViewport(x, y, x + 0.5, y + 1 / 4.f);
    renderer->SetBackground(0.5, 0.5, 0.5);
    renWin->AddRenderer(renderer);

    // add one light in front of the object
    svtkNew<svtkLight> light1;
    light1->SetPosition(0.0, 0.0, 1.0);
    light1->SetFocalPoint(0.0, 0.0, 0.0);
    light1->SetColor(1.0, 1.0, 1.0);
    light1->PositionalOn();
    light1->SwitchOn();
    renderer->AddLight(light1);

    // add three lights on the sides
    double c = cos(svtkMath::Pi() * 2.0 / 3.0);
    double s = sin(svtkMath::Pi() * 2.0 / 3.0);

    svtkNew<svtkLight> light2;
    light2->SetPosition(1.0, 0.0, 1.0);
    light2->SetFocalPoint(0.0, 0.0, 0.0);
    light2->SetColor(1.0, 1.0, 1.0);
    light2->PositionalOn();
    light2->SwitchOn();
    renderer->AddLight(light2);

    svtkNew<svtkLight> light3;
    light3->SetPosition(c, s, 1.0);
    light3->SetFocalPoint(0.0, 0.0, 0.0);
    light3->SetColor(1.0, 1.0, 1.0);
    light3->PositionalOn();
    light3->SwitchOn();
    renderer->AddLight(light3);

    svtkNew<svtkLight> light4;
    light4->SetPosition(c, -s, 1.0);
    light4->SetFocalPoint(0.0, 0.0, 0.0);
    light4->SetColor(1.0, 1.0, 1.0);
    light4->PositionalOn();
    light4->SwitchOn();
    renderer->AddLight(light4);

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphere->GetOutputPort());

    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    renderer->AddActor(actor);

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.3);
    renderer->ResetCameraClippingRange();
  }

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
