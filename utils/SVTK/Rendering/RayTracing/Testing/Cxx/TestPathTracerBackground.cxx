/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerBackground.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that environmental background options work with path tracer
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkJPEGReader.h"
#include "svtkLight.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"

#include "svtkOSPRayTestInteractor.h"

int TestPathTracerBackground(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  svtkOSPRayRendererNode::SetBackgroundMode(2, renderer);
  svtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);

  svtkSmartPointer<svtkLight> l = svtkSmartPointer<svtkLight>::New();
  l->SetLightTypeToHeadlight();
  l->SetIntensity(0.1);
  renderer->AddLight(l);

  // todo: as soon as we get materials, make the bunny reflective
  // to really show off
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkSmartPointer<svtkPLYReader> polysource = svtkSmartPointer<svtkPLYReader>::New();
  polysource->SetFileName(fileName);

  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renWin->SetSize(400, 400);

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

  // default orientation
  renderer->SetEnvironmentUp(1.0, 0.0, 0.0);
  renderer->SetEnvironmentRight(0.0, 1.0, 0.0);

  renderer->SetEnvironmentalBG(0.1, 0.1, 1.0);
  renWin->Render();
  renWin->Render(); // should cache

  renderer->SetEnvironmentalBG(0.0, 0.0, 0.0);
  renderer->SetEnvironmentalBG2(0.8, 0.8, 1.0);
  renderer->GradientEnvironmentalBGOn();
  renWin->Render(); // should invalidate and remake using default up
  renWin->Render(); // should cache

  // default view with this data is x to right, z toward camera and y up
  renderer->SetEnvironmentUp(0.0, 1.0, 0.0);
  renderer->SetEnvironmentRight(1.0, 0.0, 0.0);
  // spin up around x axis
  for (double i = 0.; i < 6.28; i += 1.)
  {
    renderer->SetEnvironmentUp(0.0, cos(i), sin(i));
    renWin->Render();
  }

  svtkSmartPointer<svtkTexture> textr = svtkSmartPointer<svtkTexture>::New();
  svtkSmartPointer<svtkJPEGReader> imgReader = svtkSmartPointer<svtkJPEGReader>::New();
  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;
  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  renderer->UseImageBasedLightingOn();
  renWin->Render(); // shouldn't crash
  renderer->SetEnvironmentTexture(textr);
  renWin->Render(); // should invalidate and remake
  renWin->Render(); // should cache
  // spin up around x axis
  for (double i = 0.; i < 6.28; i += 1.)
  {
    renderer->SetEnvironmentUp(0.0, cos(i), sin(i));
    renWin->Render();
  }

  // spin east around y axis
  for (double i = 0.; i < 6.28; i += 1.)
  {
    renderer->SetEnvironmentRight(cos(i), 0.0, sin(i));
    renWin->Render();
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
