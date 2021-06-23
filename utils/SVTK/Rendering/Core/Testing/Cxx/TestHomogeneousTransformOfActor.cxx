/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkMatrix4x4.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

int TestHomogeneousTransformOfActor(int argc, char* argv[])
{
  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetThetaResolution(10);
  sphere->SetPhiResolution(10);
  svtkPolyDataMapper* sphereMapper = svtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphere->FastDelete();

  svtkActor* sphereActor = svtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->FastDelete();

  svtkActor* referenceSphereActor = svtkActor::New();
  referenceSphereActor->SetMapper(sphereMapper);
  referenceSphereActor->SetPosition(6, 0, 0);

  // the crux of the test, set w to be not equal to 1
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  matrix->SetElement(3, 3, 0.25);
  sphereActor->SetUserMatrix(matrix);
  matrix->FastDelete();

  // Create the rendering stuff
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renderer->FastDelete();
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->FastDelete();

  renderer->AddActor(referenceSphereActor);
  referenceSphereActor->Delete();
  renderer->AddActor(sphereActor);
  sphereActor->Delete();
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(450, 450);
  renWin->Render();

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
