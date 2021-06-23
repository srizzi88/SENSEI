/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastTwoComponentsGradient.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a svtkImageData with two components.
// The data is volume rendered considering the two components as independent.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastTwoComponentsGradient(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 35, 35, 35 };

  // Create a svtkImageData with two components
  svtkNew<svtkImageData> image;
  image->SetDimensions(dims[0], dims[1], dims[2]);
  image->AllocateScalars(SVTK_DOUBLE, 2);

  // Fill the first half rectangular parallelopiped along X with the
  // first component values and the second half with second component values
  double* ptr = static_cast<double*>(image->GetScalarPointer(0, 0, 0));

  for (int z = 0; z < dims[2]; ++z)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      for (int x = 0; x < dims[0]; ++x)
      {
        if (x < dims[0] / 2)
        {
          *ptr++ = 0.0;
          *ptr++ = 0.0;
        }
        else
        {
          *ptr++ = 1.0;
          *ptr++ = 1.0;
        }
      }
    }
  }

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(301, 300); // Intentional NPOT size
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // Volume render the dataset
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.9);
  mapper->SetInputData(image);

  // Color transfer function
  svtkNew<svtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf1->AddRGBPoint(1.0, 1.0, 0.0, 0.0);

  svtkNew<svtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf2->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  // Opacity functions
  svtkNew<svtkPiecewiseFunction> pf1;
  pf1->AddPoint(0.0, 0.0);
  pf1->AddPoint(1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> pf2;
  pf2->AddPoint(0.0, 0.0);
  pf2->AddPoint(1.0, 1.0);

  // Gradient Opacity function
  svtkNew<svtkPiecewiseFunction> pf3;
  pf3->AddPoint(0.0, 0.0);
  pf3->AddPoint(1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> pf4;
  pf4->AddPoint(0.0, 0.0);
  pf4->AddPoint(1.0, 1.0);

  // Volume property with independent components ON
  svtkNew<svtkVolumeProperty> property;
  property->IndependentComponentsOn();

  // Set color and opacity functions
  property->SetColor(0, ctf1);
  property->SetColor(1, ctf2);
  property->SetScalarOpacity(0, pf1);
  property->SetScalarOpacity(1, pf2);
  property->SetGradientOpacity(0, pf3);
  property->SetGradientOpacity(1, pf4);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  ren->AddVolume(volume);

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
