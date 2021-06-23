/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultipleBackends.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can use the different raytracing backends alongside each other
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#include "svtkOSPRayTestInteractor.h"

int TestMultipleBackends(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();

  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Define viewport ranges
  double xmins[4] = { 0, .5, 0, .5 };
  double xmaxs[4] = { 0.5, 1, 0.5, 1 };
  double ymins[4] = { 0, 0, .5, .5 };
  double ymaxs[4] = { 0.5, 0.5, 1, 1 };
  for (unsigned i = 0; i < 4; i++)
  {
    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();

    renderWindow->AddRenderer(renderer);
    renderer->SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i]);

    renderer->SetBackground(0.75, 0.75, 0.75);

    if (i == 1)
    {
      // VisRTX
      svtkSmartPointer<svtkOSPRayPass> visrtxpass = svtkSmartPointer<svtkOSPRayPass>::New();
      renderer->SetPass(visrtxpass);
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
    }
    else if (i == 2)
    {
      // OSPRay
      svtkSmartPointer<svtkOSPRayPass> ospraypass = svtkSmartPointer<svtkOSPRayPass>::New();
      renderer->SetPass(ospraypass);
    }
    else if (i == 3)
    {
      // OSPRay Path Tracer
      svtkSmartPointer<svtkOSPRayPass> ospraypathtracerpass = svtkSmartPointer<svtkOSPRayPass>::New();
      renderer->SetPass(ospraypathtracerpass);
      svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
    }

    // Create a sphere
    svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
    sphereSource->SetCenter(0.0, 0.0, 0.0);
    sphereSource->SetPhiResolution(10);
    sphereSource->SetRadius(5);
    sphereSource->Update();

    // Create a mapper and actor
    svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    renderer->ResetCamera();

    renderWindow->Render();
    renderWindow->SetWindowName("Multiple ViewPorts");
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  renderWindowInteractor->SetInteractorStyle(style);

  renderWindowInteractor->Start();

  return 0;
}
