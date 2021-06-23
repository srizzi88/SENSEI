/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This test is unlikely to fail if FXAA isn't working, but can be used to
// quickly check the same scene with/without FXAA enabled.
//

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkCylinderSource.h"
#include "svtkDiskSource.h"
#include "svtkFXAAOptions.h"
#include "svtkLineSource.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"

namespace
{

void BuildRenderer(svtkRenderer* renderer, int widthBias)
{
  const size_t NUM_LINES = 10;

  svtkNew<svtkLineSource> lines[NUM_LINES];
  svtkNew<svtkPolyDataMapper> mappers[NUM_LINES];
  svtkNew<svtkActor> actors[NUM_LINES];
  for (size_t i = 0; i < NUM_LINES; ++i)
  {
    double c = static_cast<double>(2 * i) / static_cast<double>(NUM_LINES - 1) - 1.;
    lines[i]->SetPoint1(-1, c, 0.);
    lines[i]->SetPoint2(1, -c, 0.);

    mappers[i]->SetInputConnection(lines[i]->GetOutputPort());

    actors[i]->SetMapper(mappers[i]);
    actors[i]->GetProperty()->SetColor(0., 1., 0.);
    actors[i]->GetProperty()->SetRepresentationToWireframe();
    actors[i]->GetProperty()->SetLineWidth(((i + widthBias) % 2) ? 1 : 3);
    renderer->AddActor(actors[i]);
  }

  svtkNew<svtkSphereSource> sphere;
  sphere->SetCenter(0., 0.6, 0.);
  sphere->SetThetaResolution(80);
  sphere->SetPhiResolution(80);
  sphere->SetRadius(0.4);
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->GetProperty()->SetColor(0.9, 0.4, 0.2);
  sphereActor->GetProperty()->SetAmbient(.6);
  sphereActor->GetProperty()->SetDiffuse(.4);
  renderer->AddActor(sphereActor);

  svtkNew<svtkConeSource> cone;
  cone->SetCenter(0., 0.5, -0.5);
  cone->SetResolution(160);
  cone->SetRadius(.9);
  cone->SetHeight(0.9);
  cone->SetDirection(0., -1., 0.);
  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  svtkNew<svtkActor> coneActor;
  coneActor->SetMapper(coneMapper);
  coneActor->GetProperty()->SetColor(0.9, .6, 0.8);
  coneActor->GetProperty()->SetAmbient(.6);
  coneActor->GetProperty()->SetDiffuse(.4);
  renderer->AddActor(coneActor);

  svtkNew<svtkDiskSource> disk;
  disk->SetCircumferentialResolution(80);
  disk->SetInnerRadius(0);
  disk->SetOuterRadius(0.5);
  svtkNew<svtkPolyDataMapper> diskMapper;
  diskMapper->SetInputConnection(disk->GetOutputPort());
  svtkNew<svtkActor> diskActor;
  diskActor->SetPosition(0., -0.5, -0.5);
  diskActor->SetMapper(diskMapper);
  diskActor->GetProperty()->SetColor(.3, .1, .4);
  diskActor->GetProperty()->SetAmbient(.6);
  diskActor->GetProperty()->SetDiffuse(.4);
  renderer->AddActor(diskActor);

  svtkNew<svtkCylinderSource> cyl;
  cyl->SetCenter(0., -.5, 0.);
  cyl->SetHeight(.6);
  cyl->SetRadius(.2);
  cyl->SetResolution(80);
  svtkNew<svtkPolyDataMapper> cylMapper;
  cylMapper->SetInputConnection(cyl->GetOutputPort());
  svtkNew<svtkActor> cylActor;
  cylActor->SetOrigin(cyl->GetCenter());
  cylActor->RotateWXYZ(35, -0.2, 0., 1.);
  cylActor->SetMapper(cylMapper);
  cylActor->GetProperty()->SetColor(0.3, .9, .4);
  cylActor->GetProperty()->SetAmbient(.6);
  cylActor->GetProperty()->SetDiffuse(.4);
  renderer->AddActor(cylActor);

  renderer->SetBackground(0., 0., 0.);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
  renderer->GetActiveCamera()->SetParallelScale(0.9);
}

} // end anon namespace

int TestFXAAFilter(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderer> rendererFXAA;
  rendererFXAA->UseFXAAOn();

  svtkNew<svtkTextActor> label;
  label->SetInput("No FXAA");
  label->GetTextProperty()->SetFontSize(20);
  label->GetTextProperty()->SetJustificationToCentered();
  label->GetTextProperty()->SetVerticalJustificationToBottom();
  label->SetPosition(85, 10);
  renderer->AddActor2D(label);

  svtkNew<svtkTextActor> labelFXAA;
  labelFXAA->SetInput("FXAA");
  labelFXAA->GetTextProperty()->SetFontSize(20);
  labelFXAA->GetTextProperty()->SetJustificationToCentered();
  labelFXAA->GetTextProperty()->SetVerticalJustificationToBottom();
  labelFXAA->SetPosition(85, 10);
  rendererFXAA->AddActor2D(labelFXAA);

  renderer->SetViewport(0., 0., .5, 1.);
  BuildRenderer(renderer, 0);
  renWin->AddRenderer(renderer);

  rendererFXAA->SetViewport(.5, 0., 1., 1.);
  BuildRenderer(rendererFXAA, 1);
  renWin->AddRenderer(rendererFXAA);

  renWin->SetSize(1000, 500);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
