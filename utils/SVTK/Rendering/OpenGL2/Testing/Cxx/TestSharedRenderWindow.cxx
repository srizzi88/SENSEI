/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

#include "svtkOpenGLRenderWindow.h"

//----------------------------------------------------------------------------
int TestSharedRenderWindow(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetEndTheta(270.0);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  svtkNew<svtkRenderer> renderer2;
  renderer2->SetBackground(0.0, 0.0, 1.0);
  svtkNew<svtkRenderWindow> renderWindow2;
  renderWindow2->SetSize(300, 300);
  renderWindow2->AddRenderer(renderer2);
  svtkNew<svtkRenderWindowInteractor> iren2;
  iren2->SetRenderWindow(renderWindow2);
  renderWindow2->SetSharedRenderWindow(renderWindow);

  svtkNew<svtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> actor2;
  renderer2->AddActor(actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetDiffuseColor(1.0, 1.0, 0.4);

  renderWindow2->SetMultiSamples(0);
  renderer2->ResetCamera();
  renderer2->GetActiveCamera()->Elevation(-45);
  renderer2->GetActiveCamera()->OrthogonalizeViewUp();
  renderer2->GetActiveCamera()->Zoom(1.5);
  renderer2->ResetCameraClippingRange();
  renderWindow2->Render();

  int retVal = svtkRegressionTestImage(renderWindow2);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
