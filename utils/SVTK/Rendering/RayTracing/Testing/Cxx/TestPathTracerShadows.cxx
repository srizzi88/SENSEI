/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerShadows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that soft shadows work with ospray's path tracer.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkLight.h"
#include "svtkOSPRayLightNode.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkOSPRayTestInteractor.h"

int TestPathTracerShadows(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(400, 400);
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AutomaticLightCreationOff();
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->UseShadowsOn();
  svtkOSPRayRendererNode::SetSamplesPerPixel(50, renderer);
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkCamera> c = svtkSmartPointer<svtkCamera>::New();
  c->SetPosition(0, 0, 80);
  c->SetFocalPoint(0, 0, 0);
  c->SetViewUp(0, 1, 0);
  renderer->SetActiveCamera(c);

  svtkSmartPointer<svtkLight> l = svtkSmartPointer<svtkLight>::New();
  l->PositionalOn();
  l->SetPosition(4, 8, 20);
  l->SetFocalPoint(0, 0, 0);
  l->SetLightTypeToSceneLight();
  l->SetIntensity(200.0);
  renderer->AddLight(l);

  svtkSmartPointer<svtkPlaneSource> shadowee = svtkSmartPointer<svtkPlaneSource>::New();
  shadowee->SetOrigin(-10, -10, 0);
  shadowee->SetPoint1(10, -10, 0);
  shadowee->SetPoint2(-10, 10, 0);
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(shadowee->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkPlaneSource> shadower = svtkSmartPointer<svtkPlaneSource>::New();
  shadower->SetOrigin(-5, -5, 10);
  shadower->SetPoint1(5, -5, 10);
  shadower->SetPoint2(-5, 5, 10);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(shadower->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  for (double i = 0.; i < 2.0; i += 0.25)
  {
    svtkOSPRayLightNode::SetRadius(i, l);
    renWin->Render();
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
