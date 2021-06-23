/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSphereWidgetCenterCursor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkCamera.h>
#include <svtkCommand.h>
#include <svtkMath.h>
#include <svtkNew.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSphere.h>
#include <svtkSphereRepresentation.h>
#include <svtkSphereWidget2.h>

int TestSphereWidget2CenterCursor(int argc, char* argv[])
{
  // Create a renderer and a render window
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  // Create an interactor
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create a sphere widget
  svtkNew<svtkSphereWidget2> sphereWidget;
  sphereWidget->SetInteractor(renderWindowInteractor);
  sphereWidget->CreateDefaultRepresentation();

  svtkSphereRepresentation* sphereRepresentation =
    svtkSphereRepresentation::SafeDownCast(sphereWidget->GetRepresentation());
  sphereRepresentation->HandleVisibilityOff();
  double center[3] = { 4, 0, 0 };
  sphereRepresentation->SetCenter(center);
  sphereRepresentation->SetRadius(3);

  // Create a second sphere widget
  svtkNew<svtkSphereWidget2> sphereWidget2;
  sphereWidget2->SetInteractor(renderWindowInteractor);
  sphereWidget2->CreateDefaultRepresentation();

  svtkSphereRepresentation* sphereRepresentation2 =
    svtkSphereRepresentation::SafeDownCast(sphereWidget2->GetRepresentation());
  sphereRepresentation2->HandleVisibilityOff();
  double center2[3] = { -4, 0, 0 };
  sphereRepresentation2->SetCenter(center2);
  sphereRepresentation2->SetRadius(3);
  sphereRepresentation2->SetCenterCursor(true);

  svtkCamera* camera = renderer->GetActiveCamera();
  renderWindow->SetSize(300, 300);
  camera->SetPosition(0.0, 0.0, 20.0);
  camera->SetFocalPoint(0.0, 0.0, -1);

  renderWindow->Render();
  renderWindowInteractor->Initialize();
  sphereWidget->On();
  sphereWidget2->On();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
