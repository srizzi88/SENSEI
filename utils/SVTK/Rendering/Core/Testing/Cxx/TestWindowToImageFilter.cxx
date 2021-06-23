/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWindowToImageFilter.cxx

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
#include "svtkImageActor.h"
#include "svtkImageMapper3D.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkWindowToImageFilter.h"

int TestWindowToImageFilter(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(5.0);
  sphereSource->Update();

  // Render the sphere
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->Render();

  // Take the partial screenshot
  svtkNew<svtkWindowToImageFilter> windowToImageFilter;
  windowToImageFilter->SetInput(renderWindow);
  windowToImageFilter->SetInputBufferTypeToRGB();
  windowToImageFilter->SetViewport(0.5, 0.5, 0.8, 1.0);
  windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer
  windowToImageFilter->Update();

  // Show the screenshot
  svtkNew<svtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(windowToImageFilter->GetOutput());

  renderer->RemoveActor(actor);
  renderer->AddActor(imageActor);

  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
