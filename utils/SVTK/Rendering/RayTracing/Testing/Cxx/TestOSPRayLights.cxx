/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayLights.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that lighting works as expected with ospray.
// When advanced materials are exposed in ospray, it will also validate
// refractions and reflections
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
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#include "svtkOSPRayTestInteractor.h"

int TestOSPRayLights(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AutomaticLightCreationOff();
  renWin->AddRenderer(renderer);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkSmartPointer<svtkPLYReader> polysource = svtkSmartPointer<svtkPLYReader>::New();
  polysource->SetFileName(fileName);

  // measure so we can place things sensibly
  polysource->Update();
  double bds[6];
  polysource->GetOutput()->GetBounds(bds);
  double x0 = bds[0] * 2;
  double x1 = bds[1] * 2;
  double y0 = bds[2];
  double y1 = bds[3] * 2;
  double z0 = bds[4];
  double z1 = bds[5] * 4;

  // TODO: ospray acts strangely without these such that Diff and Spec are not 0..255 instead of
  // 0..1
  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecularColor(1, 1, 1);
  actor->GetProperty()->SetSpecular(0.9);
  actor->GetProperty()->SetSpecularPower(500);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkPlaneSource> backwall = svtkSmartPointer<svtkPlaneSource>::New();
  backwall->SetOrigin(x0, y0, z0);
  backwall->SetPoint1(x1, y0, z0);
  backwall->SetPoint2(x0, y1, z0);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(backwall->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecular(0);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkPlaneSource> floor = svtkSmartPointer<svtkPlaneSource>::New();
  floor->SetOrigin(x0, y0, z0);
  floor->SetPoint1(x0, y0, z1);
  floor->SetPoint2(x1, y0, z0);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(floor->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecular(0);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkPlaneSource> left = svtkSmartPointer<svtkPlaneSource>::New();
  left->SetOrigin(x0, y0, z0);
  left->SetPoint1(x0, y1, z0);
  left->SetPoint2(x0, y0, z1);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(left->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecular(0);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkSphereSource> magnifier = svtkSmartPointer<svtkSphereSource>::New();
  // TODO: use PathTracer_Dielectric material for this when available
  magnifier->SetCenter(x0 + (x1 - x0) * 0.6, y0 + (y1 - y0) * 0.2, z0 + (z1 - z0) * 0.7);
  magnifier->SetRadius((x1 - x0) * 0.05);
  magnifier->SetPhiResolution(30);
  magnifier->SetThetaResolution(30);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(magnifier->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecular(0);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkSphereSource> discoball = svtkSmartPointer<svtkSphereSource>::New();
  // TODO: use PathTracer_Metal material for this when available
  discoball->SetCenter(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0.85, z0 + (z1 - z0) * 0.5);
  discoball->SetRadius((x1 - x0) * 0.1);
  discoball->SetPhiResolution(30);
  discoball->SetThetaResolution(30);
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(discoball->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuse(1);
  actor->GetProperty()->SetSpecular(0);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkSmartPointer<svtkLight> light = svtkSmartPointer<svtkLight>::New();
  // blue light casting shadows from infinity toward bottom left back corner
  light->PositionalOff();
  light->SetPosition(x0 + (x1 - x0) * 1, y0 + (y1 - y0) * 1, z0 + (z1 + z0) * 1);
  light->SetFocalPoint(x0, y0, z0);
  light->SetLightTypeToSceneLight();
  light->SetColor(0.0, 0.0, 1.0);
  light->SetIntensity(0.3);
  light->SwitchOn();
  renderer->AddLight(light);

  light = svtkSmartPointer<svtkLight>::New();
  // red light casting shadows from top to bottom
  light->PositionalOn();
  double t = 1.8; // adjust t to see effect of positional
  light->SetPosition(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * t, z0 + (z1 - z0) * 0.5);
  light->SetFocalPoint(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0, z0 + (z1 - z0) * 0.5);
  light->SetLightTypeToSceneLight();
  light->SetColor(1.0, 0.0, 0.0);
  light->SetIntensity(0.3);
  light->SwitchOn();
  renderer->AddLight(light);

  light = svtkSmartPointer<svtkLight>::New();
  // green light following camera
  light->PositionalOn();
  light->SetLightTypeToHeadlight();
  light->SetColor(0.0, 1.0, 0.0);
  light->SetIntensity(0.3);
  light->SwitchOn();
  renderer->AddLight(light);

  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(400, 400);

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  // increase image quality from default (otherwise subsampling artifacts)
  renWin->Render();
  renderer->UseShadowsOn();
  svtkOSPRayRendererNode::SetMaxFrames(5, renderer);
  svtkOSPRayRendererNode::SetSamplesPerPixel(4, renderer);

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
