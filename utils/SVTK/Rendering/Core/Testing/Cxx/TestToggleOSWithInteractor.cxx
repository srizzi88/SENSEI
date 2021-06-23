/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSConeCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers offscreen rendering.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

// test works on Windows, in the future we need to
// make sure it works for OSX and Linux/EGL
#ifdef WIN32
int TestToggleOSWithInteractor(int argc, char* argv[])
{
  // run through a couple cases

  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(10.0);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  {
    svtkNew<svtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);

    // 1) Try calling SupportsOpenGL to make sure that
    // doesn't crash
    renderWindow->SupportsOpenGL();

    svtkNew<svtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);

    interactor->Initialize();

    // 2) try toggling offscreen rendering on and off
    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();
  }

  {
    // 3) try doing it again with a new window
    // but using existing old actor/rederer
    svtkNew<svtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);

    svtkNew<svtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);

    interactor->Initialize();

    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();

    // 4) try doing it again with offscreenbuffers
    renderWindow->OffScreenRenderingOn();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();
  }

  int retVal = 0;

  {
    // 5) try doing it again with a new everything
    svtkNew<svtkActor> actor2;
    actor2->SetMapper(mapper);
    actor2->GetProperty()->SetAmbient(1.0);
    actor2->GetProperty()->SetDiffuse(0.0);

    svtkNew<svtkRenderer> renderer2;
    renderer2->AddActor(actor2);

    svtkNew<svtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer2);

    svtkNew<svtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);

    interactor->Initialize();

    renderWindow->OffScreenRenderingOn();
    renderWindow->SupportsOpenGL();
    renderWindow->Render();
    renderWindow->OffScreenRenderingOff();
    renderWindow->Render();

    retVal = svtkRegressionTestImage(renderWindow);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      interactor->Start();
    }
  }

  return !retVal;
#else
int TestToggleOSWithInteractor(int, char*[])
{
  return 0;
#endif
}
