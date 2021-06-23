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
int TestSphereVertex(int argc, char* argv[])
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

  {
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphere->GetOutputPort());
    svtkNew<svtkActor> actor;
    renderer->AddActor(actor);
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);
    svtkNew<svtkProperty> backProp;
    backProp->SetDiffuseColor(0.4, 0.65, 0.8);
    actor->SetBackfaceProperty(backProp);

    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetLineWidth(7.0);
    actor->GetProperty()->RenderLinesAsTubesOn();

    actor->GetProperty()->VertexVisibilityOn();
    actor->GetProperty()->SetVertexColor(1.0, 0.5, 1.0);
    actor->GetProperty()->SetPointSize(14.0);
    actor->GetProperty()->RenderPointsAsSpheresOn();
  }

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
