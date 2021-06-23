/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that time varying data works as expected in ospray
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkInformation.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTimeSourceExample.h"

int TestOSPRayTime(int argc, char* argv[])
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

  svtkSmartPointer<svtkTimeSourceExample> timeywimey = svtkSmartPointer<svtkTimeSourceExample>::New();
  timeywimey->GrowingOn();
  svtkSmartPointer<svtkDataSetSurfaceFilter> dsf = svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  dsf->SetInputConnection(timeywimey->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(dsf->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  renWin->Render();
  renderer->ResetCamera();
  double pos[3];
  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(0.0, 2.5, 0.0);
  camera->GetPosition(pos);
  pos[0] = pos[0] + 6;
  pos[1] = pos[1] + 6;
  pos[2] = pos[2] + 6;
  camera->SetPosition(pos);
  renderer->ResetCameraClippingRange();
  renWin->Render();

  for (int i = 0; i < 20; i++)
  {
    double updateTime = (double)(i % 10) / 10.0;
    cerr << "t=" << updateTime << endl;
    renderer->SetActiveCamera(camera);
    svtkInformation* outInfo = dsf->GetExecutive()->GetOutputInformation(0);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    renderer->ResetCameraClippingRange();
    renWin->Render();
  }
  iren->Start();

  return 0;
}
