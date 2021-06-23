/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBlockItem.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkContextView.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestContextScene(int argc, char* argv[])
{
  // Set up a 2D context view, context test object and add it to the scene
  svtkNew<svtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(400, 400);

  svtkNew<svtkBlockItem> test;
  test->SetDimensions(20, 20, 30, 40);
  svtkNew<svtkBlockItem> test2;
  test2->SetDimensions(80, 20, 30, 40);

  svtkNew<svtkBlockItem> parent;
  parent->SetDimensions(20, 200, 80, 40);
  parent->SetLabel("Parent");
  svtkNew<svtkBlockItem> child;
  child->SetDimensions(120, 200, 80, 46);
  child->SetLabel("Child");
  svtkNew<svtkBlockItem> child2;
  child2->SetDimensions(150, 250, 86, 46);
  child2->SetLabel("Child2");

  svtkNew<svtkContextTransform> transform;
  transform->AddItem(parent);
  transform->Translate(50, -190);

  // Build up our multi-level scene
  view->GetScene()->AddItem(test);   // scene
  view->GetScene()->AddItem(test2);  // scene
  view->GetScene()->AddItem(parent); // scene
  parent->AddItem(child);            // scene->parent
  child->AddItem(child2);            // scene->parent->child

  // Add our transformed item
  view->GetScene()->AddItem(transform);

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);

  view->GetRenderWindow()->SetMultiSamples(0);

  view->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return 0;
  // return !retVal;
}
