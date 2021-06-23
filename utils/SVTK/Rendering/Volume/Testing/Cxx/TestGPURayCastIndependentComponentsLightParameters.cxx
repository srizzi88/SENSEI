/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastIndependentComponentsLightParameters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test creates a svtkImageData with three components.
// The data is volume rendered considering the three components as independent
// with shading and complex light parameters.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastIndependentComponentsLightParameters(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 100, 100, 100 };

  // Create a svtkImageData with two components
  svtkNew<svtkImageData> image;
  image->SetDimensions(dims[0], dims[1], dims[2]);
  image->AllocateScalars(SVTK_DOUBLE, 3);

  // Fill the first half rectangular parallelopiped along X with the
  // first component values and the second half with second component values
  double* ptr = static_cast<double*>(image->GetScalarPointer(0, 0, 0));

  double center1[3], center2[3], center3[3];
  center1[0] = dims[0] / 3;
  center2[0] = center1[0] * 2;
  center3[0] = dims[0] / 2;
  center1[1] = center2[1] = dims[1] / 2;
  center3[1] = dims[1] / 3;
  center1[2] = center2[2] = center3[2] = dims[2] / 2;

  double radius;
  radius = center1[0];

  svtkNew<svtkSphere> sphere1;
  sphere1->SetCenter(center1);
  sphere1->SetRadius(radius);
  svtkNew<svtkSphere> sphere2;
  sphere2->SetCenter(center2);
  sphere2->SetRadius(radius);
  svtkNew<svtkSphere> sphere3;
  sphere3->SetCenter(center3);
  sphere3->SetRadius(radius);

  for (int z = 0; z < dims[2]; ++z)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      for (int x = 0; x < dims[0]; ++x)
      {
        // Set first component
        if (sphere1->EvaluateFunction(x, y, z) > 0)
        {
          // point outside sphere 1
          *ptr++ = 0.0;
        }
        else
        {
          *ptr++ = 0.33;
        }
        // Set second component
        if (sphere2->EvaluateFunction(x, y, z) > 0)
        {
          // point outside sphere 2
          *ptr++ = 0.0;
        }
        else
        {
          *ptr++ = 0.33;
        }
        // Set third component
        if (sphere3->EvaluateFunction(x, y, z) > 0)
        {
          // point outside sphere 2
          *ptr++ = 0.0;
        }
        else
        {
          *ptr++ = 0.33;
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
  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
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
  ctf1->AddRGBPoint(1.0, 0.0, 1.0, 0.0);

  svtkNew<svtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf2->AddRGBPoint(1.0, 0.0, 1.0, 0.0);

  svtkNew<svtkColorTransferFunction> ctf3;
  ctf3->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf3->AddRGBPoint(1.0, 0.0, 1.0, 0.0);

  // Opacity functions
  svtkNew<svtkPiecewiseFunction> pf1;
  pf1->AddPoint(0.0, 0.0);
  pf1->AddPoint(1.0, 0.2);

  svtkNew<svtkPiecewiseFunction> pf2;
  pf2->AddPoint(0.0, 0.0);
  pf2->AddPoint(1.0, 0.2);

  svtkNew<svtkPiecewiseFunction> pf3;
  pf3->AddPoint(0.0, 0.0);
  pf3->AddPoint(1.0, 0.2);

  // Volume property with independent components ON
  svtkNew<svtkVolumeProperty> property;
  property->IndependentComponentsOn();

  // Set color and opacity functions
  property->SetColor(0, ctf1);
  property->SetColor(1, ctf2);
  property->SetColor(2, ctf3);
  property->SetScalarOpacity(0, pf1);
  property->SetScalarOpacity(1, pf2);
  property->SetScalarOpacity(2, pf3);

  // Define light parameters
  property->ShadeOn();

  property->SetAmbient(0, 0.2);
  property->SetDiffuse(0, 0.9);
  property->SetSpecular(0, 0.4);
  property->SetSpecularPower(0, 10.0);
  property->SetAmbient(1, 0.5);
  property->SetDiffuse(1, 0.3);
  property->SetSpecular(1, 0.1);
  property->SetSpecularPower(1, 1.0);
  property->SetAmbient(2, 0.7);
  property->SetDiffuse(2, 0.9);
  property->SetSpecular(2, 0.4);
  property->SetSpecularPower(2, 10.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  ren->AddVolume(volume);

  ren->ResetCamera();

  iren->Initialize();
  renWin->Render();

  ren->GetActiveCamera()->Zoom(1.5);

  int retVal = svtkTesting::Test(argc, argv, renWin, 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
