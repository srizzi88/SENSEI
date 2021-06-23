/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSplitViewportStereoHorizontal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test draws a cone in split-screen stereo.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkMatrix4x4.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

int TestSplitViewportStereoHorizontal(int argc, char* argv[])
{
  double bottomLeft[3] = { -1.0, -1.0, -10.0 };
  double bottomRight[3] = { 1.0, -1.0, -10.0 };
  double topRight[3] = { 1.0, 1.0, -10.0 };

  SVTK_CREATE(svtkSphereSource, sphere1);
  sphere1->SetCenter(0.2, 0.0, -7.0);
  sphere1->SetRadius(0.5);
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);

  SVTK_CREATE(svtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(sphere1->GetOutputPort());

  SVTK_CREATE(svtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(0.8, 0.8, 0.0);

  SVTK_CREATE(svtkConeSource, cone1);
  cone1->SetCenter(0.0, 0.0, -6.0);
  cone1->SetResolution(100);

  SVTK_CREATE(svtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(cone1->GetOutputPort());

  SVTK_CREATE(svtkActor, actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetAmbient(0.1);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->SetAmbient(1.0, 1.0, 1.0);

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(400, 400);
  renwin->SetStereoTypeToSplitViewportHorizontal();
  renwin->SetStereoRender(1);
  renwin->SetMultiSamples(0);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  double eyePosition[3] = { 0.0, 0.0, 2.0 };

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetScreenBottomLeft(bottomLeft);
  camera->SetScreenBottomRight(bottomRight);
  camera->SetScreenTopRight(topRight);
  camera->SetUseOffAxisProjection(1);
  camera->SetEyePosition(eyePosition);
  camera->SetEyeSeparation(0.05);
  camera->SetPosition(0.0, 0.0, 0.0);
  camera->SetFocalPoint(0.0, 0.0, -1.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetViewAngle(30.0);

  renwin->Render();

  int retVal = svtkRegressionTestImageThreshold(renwin, 25);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (!retVal);
}
