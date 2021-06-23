/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastJittering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** Tests stochastic jittering by rendering a volume exhibiting aliasing due
 * to a big sampling distance (low sampling frequency), a.k.a. wood-grain
 * artifacts. The expected output is 'filtered' due to the noise introduced
 * by jittering the entry point of the rays.
 *
 * Added renderer to expand coverage for svtkDualDepthPeelingPass.
 *
 * This test builds on TestGPURayCastJittering by rendering with jittering
 * enabled, and then without it enabled. This is to test for regressions like
 * slice bug 4600 (https://issues.slicer.org/view.php?id=4600).
 */

#include <iostream>

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderer.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSphereSource.h>
#include <svtkStructuredPointsReader.h>
#include <svtkTestUtilities.h>
#include <svtkVolume.h>
#include <svtkVolumeProperty.h>

static const char* TestGPURayCastToggleJitteringLog = "# StreamVersion 1\n"
                                                      "EnterEvent 298 27 0 0 0 0 0\n"
                                                      "MouseWheelForwardEvent 200 142 0 0 0 0 0\n"
                                                      "LeaveEvent 311 71 0 0 0 0 0\n";

int TestGPURayCastToggleJittering(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->Render(); // Create the context

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkOpenGLRenderer* oglRen = svtkOpenGLRenderer::SafeDownCast(ren);
  assert(oglRen); // This test should only be enabled for OGL2 backend.
  // This will print details about why depth peeling is unsupported:
  oglRen->SetDebug(1);
  bool supported = oglRen->IsDualDepthPeelingSupported();
  oglRen->SetDebug(0);
  if (!supported)
  {
    std::cerr << "Skipping test; volume peeling not supported.\n";
    return SVTK_SKIP_RETURN_CODE;
  }

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.svtk");
  svtkNew<svtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  // Setup actors and mappers
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);
  mapper->SetSampleDistance(2.0);
  mapper->UseJitteringOn();

  svtkNew<svtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  color->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  svtkNew<svtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(255.0, 1.0);

  svtkNew<svtkVolumeProperty> property;
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetInterpolationTypeToLinear();
  property->ShadeOff();

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  // Translucent Spheres
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(45.f, 45.f, 45.f);
  sphereSource->SetRadius(25.0);

  svtkNew<svtkActor> sphereActor;
  svtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.0, 1.0, 0.0);
  sphereProperty->SetOpacity(0.3);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  svtkNew<svtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(30.f, 30.f, 30.f);
  sphereSource2->SetRadius(25.0);

  svtkNew<svtkActor> sphereActor2;
  svtkProperty* sphereProperty2 = sphereActor2->GetProperty();
  sphereProperty2->SetColor(0.9, 0.9, 0.9);
  sphereProperty2->SetOpacity(0.3);

  svtkNew<svtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2);

  // Render window
  renWin->SetSize(800, 400);
  renWin->SetMultiSamples(0);

  // Renderer 1
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);

  ren->AddVolume(volume);
  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(115.539, 5.50485, 89.8544);
  ren->GetActiveCamera()->SetFocalPoint(32.0598, 26.5308, 28.0257);

  // Renderer 2 (translucent geometry)
  svtkNew<svtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(ren2);

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(5);
  ren2->SetUseDepthPeelingForVolumes(true);

  ren2->AddVolume(volume);
  ren2->AddActor(sphereActor);
  ren2->AddActor(sphereActor2);
  ren2->SetActiveCamera(ren->GetActiveCamera());

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  // Render with jitter enabled:
  mapper->SetUseJittering(1);
  renWin->Render();
  // And again with jitter disabled:
  mapper->SetUseJittering(0);
  renWin->Render();

  iren->Initialize();

  int rv = svtkTesting::InteractorEventLoop(argc, argv, iren, TestGPURayCastToggleJitteringLog);
  return rv;
}
