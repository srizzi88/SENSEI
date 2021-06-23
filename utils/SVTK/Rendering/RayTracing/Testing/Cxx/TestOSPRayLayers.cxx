/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayLayers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can have multiple render layers
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestOSPRayLayers(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  renWin->SetNumberOfLayers(2);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetPhiResolution(10);
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.5, 0.5, 1.0);      // should see a light blue background
  renderer->SetEnvironmentalBG(0.5, 0.5, 1.0); // should see a light blue background

  svtkSmartPointer<svtkRenderer> renderer2 = svtkSmartPointer<svtkRenderer>::New();
  renderer2->SetLayer(1);
  renWin->AddRenderer(renderer2);
  renderer2->SetBackground(1.0, 0.0, 0.0);      // should not see red background
  renderer2->SetEnvironmentalBG(1.0, 0.0, 0.0); // should not see red background
  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> mapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper2->SetInputConnection(cone->GetOutputPort());
  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  renderer2->AddActor(actor2);
  actor2->SetMapper(mapper2);

  renWin->SetSize(400, 400);
  renWin->Render();

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  svtkSmartPointer<svtkOSPRayPass> ospray2 = svtkSmartPointer<svtkOSPRayPass>::New();

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer2);
      break;
    }
  }

  renderer->SetPass(ospray);
  renderer2->SetPass(ospray2);
  renWin->Render();

  iren->Start();

  return 0;
}
