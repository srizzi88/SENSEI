/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastAdditive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers cropping on volume datasets.

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkCommand.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastCropping1(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->SetSampleDistance(0.05);

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  // Testing prefers image comparison with small images
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Intentional odd and NPOT  width/height
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Make sure we have an OpenGL context.
  renWin->Render();

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
  colorTransferFunction->AddRGBPoint(0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(255, 0.0, 1.0, 0.0);

  // Test cropping now
  volumeMapper->SetCroppingRegionPlanes(10.0, 20.0, 10.0, 20.0, 10.0, 20.0);
  volumeMapper->SetCroppingRegionFlagsToFence();
  volumeMapper->CroppingOn();

  // Setup volume actor
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(2);

  renWin->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
