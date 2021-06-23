/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideNonUniformScaleTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/// Description:
/// Test for the case when the camera is inside the bounding box of the volume
/// with a uneven scale transformation (diagonal values not same) on the prop.
/// To accentuate the issue, a large view angle is applied.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkVolume.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastCameraInsideNonUniformScaleTransform(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  svtkNew<svtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(1, 1, 1);

  double elements[16] = { 3.2, 0, 0, 200, 0, 3.2, 0, 100, 0, 0, 1.5, 40, 0, 0, 0, 1 };

  svtkNew<svtkMatrix4x4> matrix;
  matrix->DeepCopy(elements);

  // Prepare TFs
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.02);
  pf->AddPoint(1000, 0.02);
  pf->AddPoint(1150, 0.85);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.5);
  gf->AddPoint(100, 0.7);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);
  volumeProperty->ShadeOn();

  // Setup rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.1, 0.1, 0.1);

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  volume->PokeMatrix(matrix);
  ren->AddVolume(volume);

  // Prepare the camera to be inside the volume
  ren->ResetCamera();
  svtkCamera* cam = ren->GetActiveCamera();
  cam->SetViewAngle(170);
  cam->SetPosition(256.846, 168.853, 38.7375);
  cam->SetFocalPoint(178.423, 110.943, 142.038);
  cam->SetViewUp(-0.105083, 0.899357, 0.424399);
  ren->ResetCameraClippingRange();

  // Initialize interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
