/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Tests that VolumeRayCastMapper::IsCameraInside correctly detects if the
 * camera is clipping part of the proxy geometry (either by being inside the
 * bbox or by being close enough). This test positions the camera exactly at
 * a point where a corner of the proxy geometry falls behind the near plane
 * thus clipping those fragments and the volume image chunk sampled by those
 * rays.
 */

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkMetaImageReader.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastCameraInsideClipping(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren1;
  renWin->AddRenderer(ren1);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HeadMRVolume.mhd");
  svtkNew<svtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Volume
  svtkNew<svtkGPUVolumeRayCastMapper> mapper1;
  mapper1->SetInputConnection(reader->GetOutputPort());

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
  volumeProperty1->SetInterpolationTypeToLinear();
  volumeProperty1->ShadeOn();

  svtkNew<svtkVolume> volume1;
  volume1->SetMapper(mapper1);
  volume1->SetProperty(volumeProperty1);

  // Sphere
  svtkNew<svtkSphereSource> sphere;
  sphere->SetPhiResolution(20.0);
  sphere->SetThetaResolution(20.0);
  sphere->SetCenter(90, 60, 100);
  sphere->SetRadius(40.0);
  sphere->Update();
  svtkNew<svtkPolyDataMapper> pMapper;
  pMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereAct;
  sphereAct->SetMapper(pMapper);

  // Outline
  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  ren1->AddVolume(volume1);
  ren1->AddActor(sphereAct);
  ren1->AddActor(outlineActor);

  ren1->GetActiveCamera()->SetFocalPoint(94, 142, 35);
  ren1->GetActiveCamera()->SetPosition(94, 142, 200);
  ren1->GetActiveCamera()->SetViewAngle(110);
  ren1->ResetCameraClippingRange();
  renWin->Render();

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style);

  ren1->GetActiveCamera()->Elevation(-45);
  ren1->GetActiveCamera()->OrthogonalizeViewUp();

  ren1->GetActiveCamera()->Azimuth(34.9);
  ren1->GetActiveCamera()->OrthogonalizeViewUp();
  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
