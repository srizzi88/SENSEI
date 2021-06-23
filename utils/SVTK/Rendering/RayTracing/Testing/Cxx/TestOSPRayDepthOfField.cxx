/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDepthOfField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that the perspective camera's focal distance and
// aperture size work correctly.

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkJPEGReader.h"
#include "svtkLight.h"
#include "svtkOSPRayCameraNode.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"

#include "svtkOSPRayTestInteractor.h"

int TestOSPRayDepthOfField(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  svtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);
  renWin->SetSize(400, 400);

  svtkSmartPointer<svtkLight> l = svtkSmartPointer<svtkLight>::New();
  l->SetLightTypeToHeadlight();
  l->SetIntensity(1.0);
  renderer->AddLight(l);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkSmartPointer<svtkPLYReader> polysource = svtkSmartPointer<svtkPLYReader>::New();
  polysource->SetFileName(fileName);

  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());

  svtkSmartPointer<svtkActor> actor1 = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor1);
  actor1->SetMapper(mapper);
  actor1->SetPosition(0, -0.05, 0);

  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor2);
  actor2->SetMapper(mapper);
  actor2->SetPosition(0, -0.05, 0.3);

  svtkSmartPointer<svtkActor> actor3 = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor3);
  actor3->SetMapper(mapper);
  actor3->SetPosition(0, -0.05, -0.3);

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

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.3f, 0.2f, 1.0f);

  // Init focal distance
  camera->SetFocalDistance(camera->GetDistance());

  // Increase focal disk
  for (int i = 9; i < 100; i += 10)
  {
    camera->SetFocalDisk(i * 0.01f);
    renWin->Render();
  }

  // Decrease focal disk
  for (int i = 9; i < 100; i += 10)
  {
    camera->SetFocalDisk(1.0f - 0.8f * (i * 0.01f));
    renWin->Render();
  }

  // Move focal point
  for (int i = 9; i < 200; i += 10)
  {
    camera->SetFocalDistance(camera->GetDistance() + sin(i * 0.03141592653) * 0.3);
    renWin->Render();
  }

  iren->Start();
  return 0;
}
