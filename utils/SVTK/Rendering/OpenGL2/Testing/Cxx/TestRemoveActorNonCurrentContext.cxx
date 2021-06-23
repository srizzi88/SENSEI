/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRemoveActorNonCurrentContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Test for releasing graphics resources from a non-current
// render window with svtkPolyDataMapper

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"

//-----------------------------------------------------------------------------
class TestRemoveActorNonCurrentContextCallback : public svtkCommand
{
public:
  static TestRemoveActorNonCurrentContextCallback* New()
  {
    return new TestRemoveActorNonCurrentContextCallback;
  }

  void Execute(svtkObject* caller, unsigned long eventId, void* svtkNotUsed(callData)) override
  {
    if (eventId != svtkCommand::KeyPressEvent)
    {
      return;
    }

    svtkRenderWindowInteractor* interactor = static_cast<svtkRenderWindowInteractor*>(caller);
    if (interactor == nullptr)
    {
      return;
    }

    char* pressedKey = interactor->GetKeySym();

    if (strcmp(pressedKey, "9") == 0)
    {
      renderer2->RemoveAllViewProps();
      renderWindow1->Render();
      renderWindow2->Render();
    }
  }

  svtkRenderer* renderer1;
  svtkRenderer* renderer2;
  svtkRenderWindow* renderWindow1;
  svtkRenderWindow* renderWindow2;
};

//-----------------------------------------------------------------------------
int TestRemoveActorNonCurrentContext(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  svtkNew<svtkConeSource> cone;
  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  svtkNew<svtkActor> coneActor;
  coneActor->SetMapper(coneMapper);

  svtkNew<svtkRenderer> renderer1;
  svtkNew<svtkRenderWindow> renderWindow1;
  svtkNew<svtkRenderWindowInteractor> interactor1;

  renderWindow1->SetParentId(nullptr);
  renderWindow1->AddRenderer(renderer1);
  renderWindow1->SetWindowName("Victim");
  renderWindow1->SetSize(500, 300);
  renderWindow1->SetPosition(100, 100);
  interactor1->SetRenderWindow(renderWindow1);

  renderer1->AddActor(sphereActor);
  renderer1->SetBackground(1.0, 1.0, 1.0);

  // Create the second renderwindow/renderer/mapper.
  // This is the renderer we later remove all the actors from,
  // triggering the problems in the first renderer
  svtkNew<svtkRenderer> renderer2;
  svtkNew<svtkRenderWindow> renderWindow2;
  svtkNew<svtkRenderWindowInteractor> interactor2;

  renderWindow2->SetParentId(nullptr);
  renderWindow2->AddRenderer(renderer2);
  renderWindow2->SetWindowName("Villain");
  renderWindow2->SetSize(300, 300);
  renderWindow2->SetPosition(650, 100);
  interactor2->SetRenderWindow(renderWindow2);

  renderer2->AddActor(coneActor);
  renderer2->SetBackground(1.0, 1.0, 1.0);

  // Create callback so we can trigger the problem
  svtkNew<TestRemoveActorNonCurrentContextCallback> callback;
  callback->renderer1 = renderer1;
  callback->renderer2 = renderer2;
  callback->renderWindow1 = renderWindow1;
  callback->renderWindow2 = renderWindow2;
  interactor1->AddObserver("KeyPressEvent", callback);

  // Let's go
  interactor1->Initialize();
  renderWindow1->Render();
  renderWindow2->Render();
  renderWindow1->MakeCurrent();
  interactor1->SetKeyEventInformation(0, 0, 0, 0, "9");
  interactor1->InvokeEvent(svtkCommand::KeyPressEvent, nullptr);
  int retval = svtkTesting::Test(argc, argv, renderWindow1, 10);
  if (retval == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor1->Start();
  }
  return !retval;
}
