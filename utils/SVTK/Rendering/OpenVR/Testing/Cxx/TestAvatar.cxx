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
#include "svtkAvatar.h"
#include "svtkCamera.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

#include "svtkLight.h"

//----------------------------------------------------------------------------
int TestAvatar(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  svtkNew<svtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetPosition(0.4, 0, 0);
  actor->SetScale(3.0, 3.0, 3.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  svtkNew<svtkAvatar> avatar;
  avatar->SetHeadPosition(-2.4, 0.2, 0);
  avatar->SetHeadOrientation(0, 20, 0);
  avatar->SetLeftHandPosition(-0.9, -0.3, -0.7);
  avatar->SetLeftHandOrientation(-10, -20, 15);
  avatar->SetRightHandPosition(-0.6, -0.4, 0.5);
  avatar->SetRightHandOrientation(0, 0, 0);
  avatar->GetProperty()->SetColor(0.8, 1.0, 0.8);
  // avatar->SetScale(0.3);
  renderer->AddActor(avatar);

  renderer->GetActiveCamera()->SetPosition(-1.0, 0.25, 5.0);
  renderer->GetActiveCamera()->SetFocalPoint(-1.0, 0.25, 0.0);
  renderer->GetActiveCamera()->SetViewAngle(55.0);
  renderer->GetActiveCamera()->Zoom(1.1);
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(15);
  // renderer->GetActiveCamera()->Roll(-10);
  renderer->SetBackground(0.6, 0.7, 1.0);
  renderer->ResetCameraClippingRange();
  renderer->SetClippingRangeExpansion(1.5);

  renderWindow->Render();

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);
  // style->SetAutoAdjustCameraClippingRange(true);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
