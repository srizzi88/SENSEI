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
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderStepsPass.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSSAOPass.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestSSAOPass(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
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

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  svtkNew<svtkPlaneSource> plane;
  const double* plybounds = mapper->GetBounds();
  plane->SetOrigin(-0.2, plybounds[2], -0.2);
  plane->SetPoint1(-0.2, plybounds[2], 0.2);
  plane->SetPoint2(0.2, plybounds[2], -0.2);

  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  renderer->AddActor(planeActor);

  renderWindow->SetMultiSamples(0);

  svtkNew<svtkRenderStepsPass> basicPasses;

  svtkNew<svtkSSAOPass> ssao;
  ssao->SetRadius(0.05);
  ssao->SetKernelSize(128);
  ssao->SetDelegatePass(basicPasses);

  svtkOpenGLRenderer* glrenderer = svtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(ssao);

  renderer->GetActiveCamera()->SetPosition(-0.2, 0.8, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.5);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
