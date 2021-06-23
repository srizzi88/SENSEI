/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastTextureStreaming.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description:
// Tests infrastructure to volume render a 3D texture bigger than the available
// graphics memory by splitting it and individually streaming each block (bricking)
// into the GPU.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageResample.h"
#include "svtkImageResize.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

#include "svtkAbstractMapper.h"
#include "svtkImageData.h"
#include "svtkMatrix4x4.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyDataMapper.h"

int TestGPURayCastTextureStreaming(int argc, char* argv[])
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
  resample->SetOutputDimensions(512, 512, 512);
  resample->Update();

  // Prepare TFs
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
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

  // Setup rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.1, 0.1, 0.1);

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());
  mapper->SetUseJittering(0);

  // Force a number of partition blocks
  svtkOpenGLGPUVolumeRayCastMapper* mappergl = svtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
  mappergl->SetPartitions(2, 1, 2);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);

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
