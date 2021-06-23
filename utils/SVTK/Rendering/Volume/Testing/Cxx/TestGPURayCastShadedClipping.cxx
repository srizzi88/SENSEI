/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastShadedClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test covers volume shading with clipping

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPlaneCollection.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkVolume16Reader.h>
#include <svtkVolumeProperty.h>

void CreateVolumeShadedClippingPipeline(
  svtkImageData* data, svtkVolume* volume, int UseClippedVoxelIntensity)
{
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(data);
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(70.0, 0.0);
  scalarOpacity->AddPoint(1200, .2);
  scalarOpacity->AddPoint(1300, .3);
  scalarOpacity->AddPoint(2000, .3);
  scalarOpacity->AddPoint(4095.0, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);
  volumeProperty->SetClippedVoxelIntensity(-1000.0);
  volumeProperty->SetUseClippedVoxelIntensity(UseClippedVoxelIntensity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->AddRGBPoint(0.0, 0.5, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(600.0, 1.0, 0.5, 0.5);
  colorTransferFunction->AddRGBPoint(1280.0, 0.9, 0.2, 0.3);
  colorTransferFunction->AddRGBPoint(1960.0, 0.81, 0.27, 0.1);
  colorTransferFunction->AddRGBPoint(4095.0, 0.5, 0.5, 0.5);

  // Test cropping now
  const double* bounds = data->GetBounds();
  svtkNew<svtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(0.8, 0.0, 0.0);

  svtkNew<svtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.35 * (bounds[2] + bounds[3]), 0.0);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  svtkNew<svtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  // Setup volume actor
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
}

int TestGPURayCastShadedClipping(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkVolume16Reader> reader = svtkSmartPointer<svtkVolume16Reader>::New();
  reader->SetDataDimensions(64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange(1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);
  reader->Update();

  delete[] fname;

  svtkImageData* input = reader->GetOutput();

  // Testing prefers image comparison with small images
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderer> lren;
  lren->SetViewport(0, 0, 0.5, 1);
  renWin->AddRenderer(lren);
  svtkNew<svtkRenderer> rren;
  rren->SetViewport(0.5, 0, 1, 1);
  renWin->AddRenderer(rren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkVolume> lvolume;
  CreateVolumeShadedClippingPipeline(input, lvolume, 0);
  svtkNew<svtkVolume> rvolume;
  CreateVolumeShadedClippingPipeline(input, rvolume, 1);
  lren->AddViewProp(lvolume);
  lren->GetActiveCamera()->Azimuth(-40);
  lren->GetActiveCamera()->Pitch(-60);
  lren->ResetCamera();
  lren->GetActiveCamera()->Zoom(1.8);
  rren->AddViewProp(rvolume);
  rren->GetActiveCamera()->Azimuth(-40);
  rren->GetActiveCamera()->Pitch(-60);
  rren->ResetCamera();
  rren->GetActiveCamera()->Zoom(1.8);
  renWin->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
