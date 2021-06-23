/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastGradientOpacityLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests gradient opacity TF support when combined with svtkLightKit.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLightKit.h"
#include "svtkMetaImageReader.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastGradientOpacityLight(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.1, 0.4, 0.2);

  // Setup lights
  svtkNew<svtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  // Load data
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HeadMRVolume.mhd");

  svtkNew<svtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // Prepare TFs
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(1.0, 0.095, 0.33, 0.82);
  ctf->AddHSVPoint(53.3, 0.04, 0.7, 0.63);
  ctf->AddHSVPoint(256, 0.095, 0.33, 0.82);

  svtkNew<svtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(4.48, 0.0);
  pwf->AddPoint(43.116, 1.0);
  pwf->AddPoint(641.0, 1.0);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(10, 0.0);
  gf->AddPoint(70, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pwf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);
  volumeProperty->ShadeOn();

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);
  volume->RotateX(-30);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
