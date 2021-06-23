/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextActor3DDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkTextActor3D with depth peeling.
// As this actor uses svtkImageActor underneath, it also tests svtkImageActor
// with depth peeling.
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

int TestTextActor3DDepthPeeling(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

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
  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }

  renderer->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  iren->Delete();
  return !retVal;
}
