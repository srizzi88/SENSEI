/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastBlendModes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test renders a simple cube volume using different blend modes

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
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastBlendModes(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 100, 100, 100 };
  int boundary[3] = { 10, 10, 10 };

  // Create a svtkImageData with two components
  svtkNew<svtkImageData> image;
  image->SetDimensions(dims[0], dims[1], dims[2]);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  // Fill the first half rectangular parallelopiped along X with the
  // first component values and the second half with second component values
  unsigned char* ptr = static_cast<unsigned char*>(image->GetScalarPointer(0, 0, 0));

  for (int z = 0; z < dims[2]; ++z)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      for (int x = 0; x < dims[0]; ++x)
      {
        if ((z < boundary[2] || z > (dims[2] - boundary[2] - 1)) ||
          (y < boundary[1] || y > (dims[1] - boundary[1] - 1)) ||
          (x < boundary[0] || x > (dims[0] - boundary[0] - 1)))
        {
          *ptr++ = 255;
        }
        else
        {
          *ptr++ = 0;
        }
      }
    }
  }

  svtkNew<svtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.2, 0.3, 0.6);
  color->AddRGBPoint(255.0, 0.2, 0.6, 0.3);

  svtkNew<svtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(255.0, 0.8);

  svtkNew<svtkVolumeProperty> property;
  property->SetScalarOpacity(opacity);
  property->SetColor(color);

  svtkNew<svtkVolume> volume[4];

  svtkNew<svtkGPUVolumeRayCastMapper> mapper[4];
  mapper[0]->SetBlendModeToMaximumIntensity();
  mapper[1]->SetBlendModeToMinimumIntensity();
  mapper[2]->SetBlendModeToAdditive();
  mapper[3]->SetBlendModeToAverageIntensity();

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  svtkNew<svtkRenderer> renderer[4];
  renderer[0]->SetViewport(0.0, 0.0, 0.5, 0.5);
  renderer[1]->SetViewport(0.5, 0.0, 1.0, 0.5);
  renderer[2]->SetViewport(0.0, 0.5, 0.5, 1.0);
  renderer[3]->SetViewport(0.5, 0.5, 1.0, 1.0);

  for (int i = 0; i < 4; ++i)
  {
    mapper[i]->SetInputData(image);
    volume[i]->SetMapper(mapper[i]);
    volume[i]->SetProperty(property);
    renderer[i]->AddVolume(volume[i]);
    renderer[i]->SetBackground(0.3, 0.3, 0.3);
    renderer[i]->GetActiveCamera()->Yaw(20.0);
    renderer[i]->ResetCamera();
    renWin->AddRenderer(renderer[i]);
  }

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

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
