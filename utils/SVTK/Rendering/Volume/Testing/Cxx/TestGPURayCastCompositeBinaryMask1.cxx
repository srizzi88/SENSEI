/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCompositeBinaryMask1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test masks a rectangular volume to a cylindrical shape and tests that
// the mask is persistent with changing volume property parameters

#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTesting.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastCompositeBinaryMask1(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Dimensions of object
  const int cx = 128;
  const int cy = 128;
  const int cz = 512;

  // Full scale value for data
  const double fullScale = 100.0;

  // Create the image data and mask objects
  svtkNew<svtkImageData> imageData;
  imageData->SetDimensions(cx, cy, cz);
  imageData->AllocateScalars(SVTK_UNSIGNED_SHORT, 1);

  svtkNew<svtkImageData> imageMask;
  imageMask->SetDimensions(cx, cy, cz);
  imageMask->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  // Get pointers to the image and mask data's scalar arrays
  unsigned short* image = static_cast<unsigned short*>(imageData->GetScalarPointer());
  unsigned char* mask = static_cast<unsigned char*>(imageMask->GetScalarPointer());

  // Initialize image and mask with data
  int index = 0;
  for (int z = 0; z < cz; ++z)
  {
    for (int y = 0; y < cy; ++y)
    {
      for (int x = 0; x < cx; ++x)
      {
        // Data will increase from 0 to full scale in the z direction
        image[index] = static_cast<unsigned short>(fullScale * z / cz);

        // Inside the mask? Radius of cylinder mask is 1/2 cx which should
        // equal cy
        const double radius = cx / 2.0;
        const double xCenter = cx / 2.0;
        const double yCenter = cy / 2.0;
        const double distance = sqrt(pow(x - xCenter, 2.0) + pow(y - yCenter, 2.0));
        const bool inside = distance < radius;
        mask[index] = (inside) ? 255 : 0;
        index++;
      }
    }
  }

  // Create a volume mapper and add image data and mask
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputData(imageData);
  mapper->SetMaskInput(imageMask);
  mapper->SetMaskTypeToBinary();

  // Create color and opacity nodes (red and blue)
  svtkNew<svtkColorTransferFunction> colors;
  colors->AddHSVPoint(0.0 * fullScale, 0.0, 0.5, 0.5);
  colors->AddHSVPoint(1.0 * fullScale, 2.0 / 3.0, 0.5, 0.5);

  svtkNew<svtkPiecewiseFunction> opacities;
  opacities->AddPoint(0.0 * fullScale, 0.6);
  opacities->AddPoint(1.0 * fullScale, 0.6);

  // Create color property
  svtkNew<svtkVolumeProperty> colorProperty;
  colorProperty->SetColor(colors);
  colorProperty->SetScalarOpacity(opacities);

  // Create volume
  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(colorProperty);

  // Render
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  int valid = mapper->IsRenderSupported(renWin, colorProperty);
  if (!valid)
  {
    cout << "Required extensions not supported." << endl;
    return EXIT_SUCCESS;
  }

  ren->AddVolume(volume);
  renWin->Render();
  double values[6];
  colors->GetNodeValue(0, values);
  values[2] = 0.5;
  values[3] = 0.5;
  colors->SetNodeValue(0, values);
  ren->ResetCamera();
  renWin->Render();
  return svtkTesting::InteractorEventLoop(argc, argv, iren);
}
