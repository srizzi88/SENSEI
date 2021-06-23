/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBackfaceCulling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests backface culling with a text actor.
//

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextActor.h"
#include "svtkTextProperty.h"

int TestBackfaceCulling(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(300, 300);

  // Set up the sphere
  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> mapper;
  svtkNew<svtkActor> actor;
  mapper->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0, 1, 0);
  actor->GetProperty()->SetBackfaceCulling(1);
  renderer->AddActor(actor);

  // Set up the text renderer.
  svtkNew<svtkTextActor> text;
  renderer->AddActor(text);
  text->SetInput("Can you see me?");
  text->SetDisplayPosition(3, 4);

  renWin->Render();
  renderer->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
