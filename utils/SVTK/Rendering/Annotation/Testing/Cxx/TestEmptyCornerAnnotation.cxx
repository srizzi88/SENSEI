/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCornerAnnotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCornerAnnotation.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"

int TestEmptyCornerAnnotation(int argc, char* argv[])
{
  // Visualize
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->SetBackground(0.5, 0.5, 0.5);

  // Annotate the image with window/level and mouse over pixel information
  svtkSmartPointer<svtkCornerAnnotation> cornerAnnotation =
    svtkSmartPointer<svtkCornerAnnotation>::New();
  cornerAnnotation->SetLinearFontScaleFactor(2);
  cornerAnnotation->SetNonlinearFontScaleFactor(1);
  cornerAnnotation->SetMaximumFontSize(20);
  cornerAnnotation->SetText(0, "normal text");
  cornerAnnotation->SetText(1, "1234567890");
  cornerAnnotation->SetText(2, "~`!@#$%^&*()_-+=");
  cornerAnnotation->SetText(3, "text to remove");
  cornerAnnotation->GetTextProperty()->SetColor(1, 0, 0);

  renderer->AddViewProp(cornerAnnotation);

  renderWindow->Render();

  // This should empty the annotation #3 and not display a black or white box
  cornerAnnotation->SetText(3, "");
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
