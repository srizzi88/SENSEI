/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDynamicScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that dynamic scene (vary number of objects)
// contents work acceptably
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

// TODO: test broken by pre SC15 ospray caching

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#include <map>

int TestOSPRayDynamicScene(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(400, 400);
  renWin->Render();

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

#define GRIDDIM 3
  svtkSmartPointer<svtkCamera> camera = svtkSmartPointer<svtkCamera>::New();
  camera->SetPosition(GRIDDIM * 3, GRIDDIM * 3, GRIDDIM * 4);
  renderer->SetActiveCamera(camera);

  cerr << "ADD" << endl;
  std::map<int, svtkActor*> actors;
  for (int i = 0; i < GRIDDIM; i++)
  {
    for (int j = 0; j < GRIDDIM; j++)
    {
      for (int k = 0; k < GRIDDIM; k++)
      {
        svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
        sphere->SetCenter(i, j, k);
        sphere->SetPhiResolution(10);
        sphere->SetThetaResolution(10);
        svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());
        svtkActor* actor = svtkActor::New();
        renderer->AddActor(actor);
        actor->SetMapper(mapper);
        actors[i * GRIDDIM * GRIDDIM + j * GRIDDIM + k] = actor;
        renWin->Render();
      }
    }
  }

  cerr << "HIDE" << endl;
  for (int i = 0; i < GRIDDIM; i++)
  {
    for (int j = 0; j < GRIDDIM; j++)
    {
      for (int k = 0; k < GRIDDIM; k++)
      {
        svtkActor* actor = actors[i * GRIDDIM * GRIDDIM + j * GRIDDIM + k];
        actor->VisibilityOff();
        renWin->Render();
      }
    }
  }

  cerr << "SHOW" << endl;
  for (int i = 0; i < GRIDDIM; i++)
  {
    for (int j = 0; j < GRIDDIM; j++)
    {
      for (int k = 0; k < GRIDDIM; k++)
      {
        svtkActor* actor = actors[i * GRIDDIM * GRIDDIM + j * GRIDDIM + k];
        actor->VisibilityOn();
        renWin->Render();
      }
    }
  }

  cerr << "REMOVE" << endl;
  for (int i = 0; i < GRIDDIM; i++)
  {
    for (int j = 0; j < GRIDDIM; j++)
    {
      for (int k = 0; k < GRIDDIM; k++)
      {
        svtkActor* actor = actors[i * GRIDDIM * GRIDDIM + j * GRIDDIM + k];
        // leaving one to have a decent image to compare against
        bool killme = !(i == 0 && j == 1 && k == 0);
        if (killme)
        {
          renderer->RemoveActor(actor);
          actor->Delete();
          renWin->Render();
        }
      }
    }
  }

  iren->Start();

  renderer->RemoveActor(actors[0 * GRIDDIM * GRIDDIM + 1 * GRIDDIM + 0]);
  actors[0 * GRIDDIM * GRIDDIM + 1 * GRIDDIM + 0]->Delete();

  return 0;
}
