/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor3DAlphaBlending.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkTextActor3D with default alpha blending.
// As this actor uses svtkImageActor underneath, it also tests svtkImageActor
// with alpha blending.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextActor3D.h"
#include "svtkTextProperty.h"

int TestTextActor3DAlphaBlending(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(300, 300);

  svtkTextActor3D* actor = svtkTextActor3D::New();
  renderer->AddActor(actor);
  actor->Delete();

  actor->SetInput("0123456789.");

  svtkTextProperty* textProperty = svtkTextProperty::New();
  actor->SetTextProperty(textProperty);
  textProperty->Delete();

  actor->SetPosition(3, 4, 5);
  actor->SetScale(0.05, 0.05, 1);
  textProperty->SetJustificationToCentered();
  textProperty->SetVerticalJustificationToCentered(); // default
  textProperty->SetFontFamilyToArial();               // default.

  renWin->Render();
  renderer->ResetCamera();

  renWin->Render();

  // usual font issues so we up the tolerance a bit
  int retVal = svtkTesting::Test(argc, argv, renWin, 0.17);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  iren->Delete();
  return !retVal;
}
