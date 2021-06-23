/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

namespace
{

void InitRenderer(svtkRenderer* renderer)
{
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(8);
  renderer->LightFollowCameraOn();
  renderer->TwoSidedLightingOn();
  renderer->SetOcclusionRatio(0.0);
}

} // end anon namespace

int TestDepthPeelingPassViewport(int, char*[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(10);

  svtkNew<svtkRenderer> renderer;
  InitRenderer(renderer);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetAlphaBitPlanes(1);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderer> renderer2;
  InitRenderer(renderer2);
  renderer2->SetViewport(0.0, 0.1, 0.2, 0.3);
  renderer2->InteractiveOff();
  renWin->AddRenderer(renderer2);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(0.0, 0.0, 1.0);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    svtkProperty* prop = actor->GetProperty();
    prop->SetAmbientColor(1.0, 0.0, 0.0);
    prop->SetDiffuseColor(1.0, 0.8, 0.3);
    prop->SetSpecular(0.0);
    prop->SetDiffuse(0.5);
    prop->SetAmbient(0.3);
    renderer2->AddActor(actor);
  }
  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(10.0, 0.0, 0.0);
    renderer2->AddActor(actor);
  }

  renderer->SetLayer(0);
  renderer2->SetLayer(1);
  renWin->SetNumberOfLayers(2);

  renderer->ResetCamera();
  renderer2->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
