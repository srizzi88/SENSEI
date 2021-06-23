/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideRotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/// Description:
/// Tests clipping of a rotated volume (svtkProp3D::GetMatrix) using the camera's
/// near plane while the camera is inside the volume.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageResize.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

static const char* TestGPURayCastCameraInsideTransformationLog =
  "# StreamVersion 1\n"
  "EnterEvent 298 27 0 0 0 0 0\n"
  "MouseWheelForwardEvent 200 142 0 0 0 0 0\n"
  "LeaveEvent 311 71 0 0 0 0 0\n";

int TestGPURayCastCameraInsideTransformation(int argc, char* argv[])
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
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.1, 0.1, 0.1);

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  // Set a svtkProp3D transformation
  volume->RotateX(180);
  volume->RotateY(85);
  volume->RotateZ(55);
  volume->SetOrigin(300, 20, 30);

  // Prepare the camera to be inside the volume
  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(308.423, 120.943, -142.038);

  // Initialize interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render();
  iren->Initialize();

  int rv =
    svtkTesting::InteractorEventLoop(argc, argv, iren, TestGPURayCastCameraInsideTransformationLog);
  return rv;
}
