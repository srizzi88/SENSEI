/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastImageSampleXY.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * \brief Tests image sample distance (XY resolution) of a volume (ray-cast)
 * rendering.
 *
 */

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkConeSource.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageResize.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastImageSampleXY(int argc, char* argv[])
{
  // cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  svtkNew<svtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(3.2, 3.2, 1.5);

  // Upsample data
  svtkNew<svtkImageResize> resample;
  resample->SetInputConnection(reader->GetOutputPort());
  resample->SetResizeMethodToOutputDimensions();
  resample->SetOutputDimensions(128, 128, 128);
  resample->Update();

  // Setup mappers, properties and actors
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 0.1, 1.0, 0.3);
  ctf->AddRGBPoint(1000, 0.1, 1.0, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.15);
  pf->AddPoint(1000, 0.15);
  pf->AddPoint(1150, 0.85);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.5);
  gf->AddPoint(100, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  // Downsample volume-rendered image (cast 1 ray for a 4x4 pixel kernel)
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());
  mapper->SetUseJittering(0);
  mapper->SetImageSampleDistance(8.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  // Without down-sampling
  svtkNew<svtkGPUVolumeRayCastMapper> mapper2;
  mapper2->SetInputConnection(resample->GetOutputPort());
  mapper2->SetUseJittering(0);
  mapper2->SetImageSampleDistance(1.0);

  svtkNew<svtkVolume> volume2;
  volume2->SetMapper(mapper2);
  volume2->SetProperty(volumeProperty);

  svtkNew<svtkConeSource> coneSource;
  coneSource->SetResolution(20);
  coneSource->SetHeight(280);
  coneSource->SetRadius(40.0);
  coneSource->SetCenter(110.0, 70.0, 30.0);
  coneSource->Update();

  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());

  svtkNew<svtkActor> coneActor;
  coneActor->SetMapper(coneMapper);

  // Setup rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.5);
  ren->SetViewport(0.0, 0.0, 0.5, 0.5);
  ren->AddVolume(volume);
  ren->AddActor(coneActor);
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderer> ren2;
  ren2->SetBackground(0., 0., 0.);
  ren2->SetViewport(0.0, 0.5, 0.5, 1.0);
  ren2->SetActiveCamera(ren->GetActiveCamera());
  ren2->AddVolume(volume);
  ren2->AddActor(coneActor);
  renWin->AddRenderer(ren2);

  svtkNew<svtkRenderer> ren3;
  ren3->SetBackground(0., 0., 0.);
  ren3->SetViewport(0.5, 0.0, 1.0, 0.5);
  ren3->SetActiveCamera(ren->GetActiveCamera());
  ren3->AddVolume(volume);
  ren3->AddActor(coneActor);
  renWin->AddRenderer(ren3);

  svtkNew<svtkRenderer> ren4;
  ren4->SetBackground(0.3, 0.3, 0.5);
  ren4->SetViewport(0.5, 0.5, 1.0, 1.0);
  ren4->SetActiveCamera(ren->GetActiveCamera());
  ren4->AddVolume(volume2);
  ren4->AddActor(coneActor);
  renWin->AddRenderer(ren4);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-10);
  ren->GetActiveCamera()->Elevation(130);
  ren->GetActiveCamera()->Zoom(1.6);

  // Interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
