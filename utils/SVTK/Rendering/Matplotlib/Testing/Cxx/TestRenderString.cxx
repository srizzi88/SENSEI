/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMathTextRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMathTextUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageViewer2.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestRenderString(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  const char* str = "$\\hat{H}\\psi = \\left(-\\frac{\\hbar}{2m}\\nabla^2"
                    " + V(r)\\right) \\psi = \\psi\\cdot E $";

  svtkNew<svtkImageData> image;
  svtkNew<svtkMathTextUtilities> utils;
  utils->SetScaleToPowerOfTwo(false);
  svtkNew<svtkTextProperty> tprop;
  tprop->SetColor(1, 1, 1);
  tprop->SetFontSize(50);

  svtkNew<svtkImageViewer2> viewer;
  utils->RenderString(str, image, tprop, viewer->GetRenderWindow()->GetDPI());

  viewer->SetInputData(image);

  svtkNew<svtkRenderWindowInteractor> iren;
  viewer->SetupInteractor(iren);

  viewer->Render();
  viewer->GetRenderer()->ResetCamera();
  viewer->GetRenderer()->GetActiveCamera()->Zoom(6.0);
  viewer->Render();

  viewer->GetRenderWindow()->SetMultiSamples(0);
  viewer->GetRenderWindow()->GetInteractor()->Initialize();
  viewer->GetRenderWindow()->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
