/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDynamicObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can render dynamic objects (changing mesh)
// and that changing svtk state changes the resulting image accordingly.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

// TODO: test broken by pre SC15 ospray caching

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestOSPRayDynamicObject(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetPhiResolution(100);
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1, 0.1, 1.0);
  renderer->SetEnvironmentalBG(0.1, 0.1, 1.0);
  renWin->SetSize(400, 400);
  renWin->Render();

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();

  renderer->SetPass(ospray);
  renWin->Render();

  svtkLight* light = svtkLight::SafeDownCast(renderer->GetLights()->GetItemAsObject(0));
  double lColor[3];
  lColor[0] = 0.5;
  lColor[1] = 0.5;
  lColor[2] = 0.5;
  light->SetDiffuseColor(lColor[0], lColor[1], lColor[2]);

  svtkCamera* camera = renderer->GetActiveCamera();
  double position[3];
  camera->GetPosition(position);
  camera->SetClippingRange(0.01, 1000.0);

#define MAXFRAME 20
  double inc = 1.0 / (double)MAXFRAME;

  for (int i = 0; i < MAXFRAME; i++)
  {
    double I = (double)i / (double)MAXFRAME;
    renWin->SetSize(400 + i, 400 - i);
    sphere->SetThetaResolution(3 + i);

    lColor[0] += inc / 2;
    lColor[1] -= inc / 2;
    light->SetDiffuseColor(lColor[0], lColor[1], lColor[2]);

    if (i < (MAXFRAME / 2))
    {
      position[2] += inc * 5;
    }
    else
    {
      position[2] -= inc * 5;
    }

    camera->SetPosition(position);

    renderer->SetBackground(0.0, I, 1 - I);
    renderer->SetEnvironmentalBG(0.0, I, 1 - I);
    renWin->Render();
  }

  iren->Start();

  return 0;
}
