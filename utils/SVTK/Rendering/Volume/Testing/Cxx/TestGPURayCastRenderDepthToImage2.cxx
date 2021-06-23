/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastRenderDepthToImage2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test the GPU volume mapper low level API to render depth buffer to texture

#include "svtkActor.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToColors.h"
#include "svtkImageMapper3D.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastRenderDepthToImage2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  svtkNew<svtkRTAnalyticSource> waveletSource;
  volumeMapper->SetInputConnection(waveletSource->GetOutputPort());
  volumeMapper->RenderToImageOn();
  volumeMapper->SetClampDepthToBackface(1);

  svtkNew<svtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(37.35310363769531, 0.231373, 0.298039, 0.752941);
  colorFunction->AddRGBPoint(157.0909652709961, 0.865003, 0.865003, 0.865003);
  colorFunction->AddRGBPoint(276.8288269042969, 0.705882, 0.0156863, 0.14902);

  float dataRange[2];
  dataRange[0] = 37.3;
  dataRange[1] = 276.8;
  float halfSpread = (dataRange[1] - dataRange[0]) / 2.0;
  float center = dataRange[0] + halfSpread;

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->RemoveAllPoints();
  scalarOpacity->AddPoint(center, 0.0);
  scalarOpacity->AddPoint(dataRange[1], 0.4);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetColor(colorFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  // Setup volume actor
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Testing prefers image comparison with small images
  svtkNew<svtkRenderWindow> renWin;

  // Intentional odd and NPOT  width/height
  renWin->SetSize(401, 399);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->AddVolume(volume);
  ren->ResetCamera();
  renWin->Render();

  svtkNew<svtkImageData> im;

  // Get color texture as image
  volumeMapper->GetColorImage(im);

  // Get depth texture as image
  volumeMapper->GetDepthImage(im);

  // Create a grayscale lookup table
  svtkNew<svtkLookupTable> lut;
  lut->SetRange(0.0, 1.0);
  lut->SetValueRange(0.0, 1.0);
  lut->SetSaturationRange(0.0, 0.0);
  lut->SetRampToLinear();
  lut->Build();

  // Map the pixel values of the image with the lookup table
  svtkNew<svtkImageMapToColors> imageMap;
  imageMap->SetInputData(im);
  imageMap->SetLookupTable(lut);

  // Render the image in the scene
  svtkNew<svtkImageActor> ia;
  ia->GetMapper()->SetInputConnection(imageMap->GetOutputPort());
  ren->AddActor(ia);
  ren->RemoveVolume(volume);
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
