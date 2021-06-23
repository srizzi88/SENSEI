/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLightingMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test ... TO DO
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCameraPass.h"
#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkLight.h"
#include "svtkLightingMapPass.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderPassCollection.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSequencePass.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

int TestLightingMapNormalsPass(int argc, char* argv[])
{
  bool interactive = false;

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
    }
  }

  // 0. Prep data
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkSmartPointer<svtkPLYReader> reader = svtkSmartPointer<svtkPLYReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
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

  // 1. Set up renderer, window, & interactor
  svtkSmartPointer<svtkRenderWindowInteractor> interactor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  svtkSmartPointer<svtkRenderWindow> window = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

  window->AddRenderer(renderer);
  interactor->SetRenderWindow(window);

  svtkSmartPointer<svtkLight> light = svtkSmartPointer<svtkLight>::New();
  light->SetLightTypeToSceneLight();
  light->SetPosition(0.0, 0.0, 1.0);
  light->SetPositional(true);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetIntensity(1.0);

  renderer->AddLight(light);

  renderer->AddActor(actor);

  // 2. Set up rendering passes
  svtkSmartPointer<svtkLightingMapPass> lightingPass = svtkSmartPointer<svtkLightingMapPass>::New();
  lightingPass->SetRenderType(svtkLightingMapPass::NORMALS);

  svtkSmartPointer<svtkRenderPassCollection> passes = svtkSmartPointer<svtkRenderPassCollection>::New();
  passes->AddItem(lightingPass);

  svtkSmartPointer<svtkSequencePass> sequence = svtkSmartPointer<svtkSequencePass>::New();
  sequence->SetPasses(passes);

  svtkSmartPointer<svtkCameraPass> cameraPass = svtkSmartPointer<svtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  svtkOpenGLRenderer* glRenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glRenderer->SetPass(cameraPass);

  // 3. Render image and compare against baseline
  window->Render();

  int retVal = svtkRegressionTestImage(window);
  if (interactive || retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !retVal;
}
