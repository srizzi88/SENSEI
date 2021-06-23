/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FrustumClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkClipPolyData.h"
#include "svtkDebugLeaks.h"
#include "svtkPlanes.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

// Generate a sphere. Create a view frustum looking at the sphere
// Clip anything inside the frustum, then back away and view result

int FrustumClip(int argc, char* argv[])
{
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetThetaResolution(36);
  sphere->SetPhiResolution(18);
  sphere->SetRadius(1);

  svtkPolyDataMapper* sphereMapper = svtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  svtkActor* sphereActor = svtkActor::New();
  sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);
  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(400, 300);

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(1.5, 0.0, 0.0);
  renderer->ResetCameraClippingRange();

  // Display once with camera in position 1
  // Ensures clipping planes are initialized (camera matrix really).
  renWin->Render();

  // Now get the camera frustum and then move the camera away to see the
  // clipped away stuff
  double aspect = 400.0 / 300.0, planeequations[24];
  camera->GetFrustumPlanes(aspect, planeequations);

  svtkPlanes* implictplanes = svtkPlanes::New();
  // TODO cleanup
  double ped[24];
  int i;
  for (i = 0; i < 24; ++i)
  {
    ped[i] = planeequations[i];
  }
  implictplanes->SetFrustumPlanes(ped);

  svtkClipPolyData* clipper = svtkClipPolyData::New();
  clipper->SetInputConnection(sphere->GetOutputPort());
  clipper->SetClipFunction(implictplanes);
  clipper->SetGenerateClipScalars(1);
  clipper->SetInsideOut(0);
  sphereMapper->SetInputConnection(clipper->GetOutputPort());

  camera->SetPosition(-4.0, 0.25, 0.25);
  renderer->ResetCameraClippingRange();

  sphereActor->GetProperty()->SetColor(0.0, 0.0, 0.0);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  implictplanes->Delete();
  clipper->Delete();

  return !retVal;
}
