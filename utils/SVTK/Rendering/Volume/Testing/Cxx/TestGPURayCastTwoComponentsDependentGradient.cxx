/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastTwoComponentsDependentGradient.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a svtkImageData with two components.
// The data is volume rendered considering the two components as dependent
// and gradient based modulation of the opacity is applied

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

int TestGPURayCastTwoComponentsDependentGradient(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 30, 30, 30 };

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
          if (y < dims[1] / 2)
          {
            *ptr++ = 0.0;
            *ptr++ = 0.0;
          }
          else
          {
            *ptr++ = 0.25;
            *ptr++ = 25.0;
          }
        }
        else
        {
          if (y < dims[1] / 2)
          {
            *ptr++ = 0.5;
            *ptr++ = 50.0;
          }
          else
          {
            *ptr++ = 1.0;
            *ptr++ = 100.0;
          }
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
  mapper->SetSampleDistance(0.5);
  mapper->SetInputData(image);

  // Color transfer function
  svtkNew<svtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  ctf1->AddRGBPoint(0.5, 0.0, 1.0, 0.0);
  ctf1->AddRGBPoint(1.0, 1.0, 0.0, 0.0);

  svtkNew<svtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf2->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  // Opacity functions
  svtkNew<svtkPiecewiseFunction> pf1;
  pf1->AddPoint(0.0, 0.1);
  pf1->AddPoint(100.0, 0.1);

  // Gradient Opacity function
  // Whenever the gradient
  svtkNew<svtkPiecewiseFunction> pf2;
  pf2->AddPoint(0.0, 0.2);
  pf2->AddPoint(30.0, 1.0);

  // Volume property with independent components OFF
  svtkNew<svtkVolumeProperty> property;
  property->IndependentComponentsOff();

  // Set color and opacity functions
  property->SetColor(0, ctf1);
  // Setting the transfer function for second component would be a no-op as only
  // the first component functions are used.
  property->SetColor(1, ctf2);
  property->SetScalarOpacity(0, pf1);
  property->SetGradientOpacity(0, pf2);

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
