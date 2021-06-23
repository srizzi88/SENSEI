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

#include "svtkActor.h"
#include "svtkConeSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestOnAndOffScreenConeCxx(int argc, char* argv[])
{
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  // renWin->SetShowWindow(false);
  // renWin->OffScreenRenderingOn();
  renWin->SetMultiSamples(0);

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  svtkConeSource* cone = svtkConeSource::New();
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(cone->GetOutputPort());
  cone->Delete();

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();

  renderer->AddActor(actor);
  actor->Delete();

  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();
  renWin->SetShowWindow(false);
  renWin->SetUseOffScreenBuffers(true);
  renderer->SetBackground(0, 0, 0);

  renWin->Render();
  renWin->Render();
  renWin->Render();
  renWin->Render();

#if 0
  svtkRenderWindowInteractor *iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  renWin->Render();

  int retVal = svtkRegressionTestImage( renWin );
  if ( retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  iren->Delete();
#else // the interactor version fails with OSMesa.
  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  renWin->Delete();
#endif
  return !retVal;
}
