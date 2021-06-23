/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestActorLightingFlag.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the lighting flag on a svtkProperty object of an svtkActor.
// It draws a cone with lighting next to a cone with no lighting, next to a
// third cone with lighting again.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkCamera.h"
#include "svtkPolyDataMapper.h"

#include "svtkConeSource.h"
#include "svtkProperty.h"

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(svtkRenderer* r);

int TestActorLightingFlag(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  svtkConeSource* coneSource1 = svtkConeSource::New();
  svtkPolyDataMapper* coneMapper1 = svtkPolyDataMapper::New();
  coneMapper1->SetInputConnection(coneSource1->GetOutputPort());
  coneSource1->Delete();
  svtkActor* coneActor1 = svtkActor::New();
  coneActor1->SetMapper(coneMapper1);
  coneMapper1->Delete();
  coneActor1->SetPosition(-2.0, 0.0, 0.0);
  renderer->AddActor(coneActor1);
  coneActor1->Delete();

  svtkConeSource* coneSource2 = svtkConeSource::New();
  svtkPolyDataMapper* coneMapper2 = svtkPolyDataMapper::New();
  coneMapper2->SetInputConnection(coneSource2->GetOutputPort());
  coneSource2->Delete();
  svtkActor* coneActor2 = svtkActor::New();
  coneActor2->SetMapper(coneMapper2);
  coneMapper2->Delete();
  coneActor2->SetPosition(0.0, 0.0, 0.0);
  coneActor2->GetProperty()->SetLighting(false);
  renderer->AddActor(coneActor2);
  coneActor2->Delete();

  svtkConeSource* coneSource3 = svtkConeSource::New();
  svtkPolyDataMapper* coneMapper3 = svtkPolyDataMapper::New();
  coneMapper3->SetInputConnection(coneSource3->GetOutputPort());
  coneSource3->Delete();
  svtkActor* coneActor3 = svtkActor::New();
  coneActor3->SetMapper(coneMapper3);
  coneMapper3->Delete();
  coneActor3->SetPosition(2.0, 0.0, 0.0);
  renderer->AddActor(coneActor3);
  coneActor3->Delete();

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(200, 200);

  renWin->Render();

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
