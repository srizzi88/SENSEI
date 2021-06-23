/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers cropping on volume datasets.

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPlaneCollection.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkVolumeProperty.h>

int TestGPURayCastClipping(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->Update();
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  // Testing prefers image comparison with small images
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.1, 0.5, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 0.5, 0.1);

  // Test cropping now
  const double* bounds = wavelet->GetOutput()->GetBounds();
  svtkNew<svtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(0.8, 0.0, 0.0);

  svtkNew<svtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.35 * (bounds[2] + bounds[3]), 0.0);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  svtkNew<svtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  // Setup volume actor
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->GetActiveCamera()->Azimuth(-40);
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
