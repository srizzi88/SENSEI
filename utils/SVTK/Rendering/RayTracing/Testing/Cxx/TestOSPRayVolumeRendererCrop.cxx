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
// This test checks if direct ospray volume mapper intermixes with
// surface geometry in the scene.

#include <svtkCamera.h>
#include <svtkClipPolyData.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOSPRayPass.h>
#include <svtkOSPRayVolumeMapper.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
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

#include <svtkAutoInit.h>
SVTK_MODULE_INIT(svtkRenderingRayTracing);

int TestOSPRayVolumeRendererCrop(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkActor> dssActor;
  svtkNew<svtkPolyDataMapper> dssMapper;
  svtkNew<svtkOSPRayVolumeMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);

  volumeMapper->SetInputConnection(reader->GetOutputPort());

  double planes[6] = { 0.0, 57.0, 0.0, 100.0, 0.0, 74.0 };
  volumeMapper->SetCroppingRegionPlanes(planes);
  volumeMapper->CroppingOn();

  reader->Update();
  volumeMapper->GetInput()->GetScalarRange(scalarRange);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.1);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.8, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.0, 0.8, 0.1);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->AddActor(dssActor);
  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = svtkRegressionTestImageThreshold(renWin, 50.0);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
