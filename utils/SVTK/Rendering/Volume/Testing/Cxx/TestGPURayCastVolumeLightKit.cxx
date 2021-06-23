/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeRotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkLightKit.h>
#include <svtkNew.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkStructuredPointsReader.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastVolumeLightKit(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  delete[] volumeFile;

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.1);

  svtkNew<svtkLightKit> lightKit;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.0, 0.0, 0.0);
  ren->SetTwoSidedLighting(0);

  lightKit->SetKeyLightWarmth(1.0);
  lightKit->SetFillLightWarmth(0.0);
  lightKit->SetBackLightWarmth(0.0);
  lightKit->AddLightsToRenderer(ren);

  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(55, 0.0);
  scalarOpacity->AddPoint(65, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.0);
  volumeProperty->SetDiffuse(1.0);
  volumeProperty->SetSpecular(0.0);
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren->AddViewProp(volume);

  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
