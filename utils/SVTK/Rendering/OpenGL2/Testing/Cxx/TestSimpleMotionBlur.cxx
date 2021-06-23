/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderStepsPass.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimpleMotionBlurPass.h"
#include "svtkTestUtilities.h"
#include "svtkTextureObject.h"

//----------------------------------------------------------------------------
int TestSimpleMotionBlur(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(500, 500);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // create three dragons
  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(1.0, 0.0, 0.0);
    actor->GetProperty()->SetDiffuseColor(1.0, 0.8, 0.3);
    actor->GetProperty()->SetSpecular(0.0);
    actor->GetProperty()->SetDiffuse(0.5);
    actor->GetProperty()->SetAmbient(0.3);
    actor->SetPosition(-0.1, 0.0, -0.1);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
    actor->GetProperty()->SetDiffuseColor(0.2, 1.0, 0.8);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.2);
    actor->GetProperty()->SetDiffuse(0.9);
    actor->GetProperty()->SetAmbient(0.1);
    actor->GetProperty()->SetSpecularPower(10.0);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.5, 0.65, 1.0);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.7);
    actor->GetProperty()->SetDiffuse(0.4);
    actor->GetProperty()->SetSpecularPower(60.0);
    actor->SetPosition(0.1, 0.0, 0.1);
    renderer->AddActor(actor);
  }

  renderWindow->SetMultiSamples(0);

  // create the basic SVTK render steps
  svtkNew<svtkRenderStepsPass> basicPasses;

  svtkNew<svtkSimpleMotionBlurPass> motion;
  motion->SetDelegatePass(basicPasses);

  // tell the renderer to use our render pass pipeline
  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(motion);

  int numRenders = 30;
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(15.0);
  renderer->GetActiveCamera()->Zoom(1.8);
  renderWindow->Render();

  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(10.0 / numRenders);
    renderer->GetActiveCamera()->Elevation(10.0 / numRenders);
    renderWindow->Render();
  }

  int retVal = svtkRegressionTestImageThreshold(renderWindow, 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
