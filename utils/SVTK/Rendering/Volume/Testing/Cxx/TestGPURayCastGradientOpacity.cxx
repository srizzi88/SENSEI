/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastGradientOpacity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This code volume renders the torso dataset and tests the gradient opacity
// function support for volume mappers

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkInteractorStyleTrackballCamera.h"
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

int TestGPURayCastGradientOpacity(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren1;
  ren1->SetViewport(0.0, 0.0, 0.5, 1.0);
  renWin->AddRenderer(ren1);
  svtkNew<svtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(ren2);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HeadMRVolume.mhd");

  svtkNew<svtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkNew<svtkGPUVolumeRayCastMapper> mapper1;
  mapper1->SetInputConnection(reader->GetOutputPort());
  svtkNew<svtkGPUVolumeRayCastMapper> mapper2;
  mapper2->SetInputConnection(reader->GetOutputPort());

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

  svtkNew<svtkVolumeProperty> volumeProperty1;
  volumeProperty1->SetScalarOpacity(pwf);
  volumeProperty1->SetColor(ctf);
  volumeProperty1->SetDisableGradientOpacity(1);
  volumeProperty1->ShadeOn();

  svtkNew<svtkVolume> volume1;
  volume1->SetMapper(mapper1);
  volume1->SetProperty(volumeProperty1);
  ren1->AddVolume(volume1);
  volume1->RotateX(-20);
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Zoom(2.2);

  svtkNew<svtkVolumeProperty> volumeProperty2;
  volumeProperty2->SetScalarOpacity(pwf);
  volumeProperty2->SetColor(ctf);
  volumeProperty2->SetGradientOpacity(gf);
  volumeProperty2->SetDisableGradientOpacity(0);
  volumeProperty2->ShadeOn();

  svtkNew<svtkVolume> volume2;
  volume2->SetMapper(mapper2);
  volume2->SetProperty(volumeProperty2);
  volume2->RotateX(-20);
  ren2->AddVolume(volume2);
  ren2->ResetCamera();
  ren2->GetActiveCamera()->Zoom(2.2);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
