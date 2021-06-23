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
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
// Test one can create an resize offscreen render windows.
int TestOffscreenRenderingResize(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> window;
  window->SetShowWindow(false);
  window->SetUseOffScreenBuffers(true);
  window->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(window);

  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.3);
  window->AddRenderer(ren);

  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort(0));
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  ren->ResetCamera();
  window->Render();

  window->SetSize(400, 300);
  window->Render();
  int retVal = svtkRegressionTestImage(window);
  if (retVal == svtkTesting::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
