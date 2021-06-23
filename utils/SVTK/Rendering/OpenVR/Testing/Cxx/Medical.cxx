/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCamera.h"
#include "svtkRenderer.h"

#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTimerLog.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include "svtkDICOMImageReader.h"

#include "svtkActor.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCullerCollection.h"
#include "svtkImageData.h"
#include "svtkImageShrink3D.h"
#include "svtkLight.h"

#define USE_VIVE
#ifdef USE_VIVE
#include "svtkOpenVRCamera.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkOpenVRRenderer.h"
#else
#include "svtkOpenGLCamera.h"
#include "svtkOpenGLRenderer.h"
#include "svtkWin32OpenGLRenderWindow.h"
#include "svtkWin32RenderWindowInteractor.h"
#endif

//----------------------------------------------------------------------------
int Medical(int argc, char* argv[])
{
#ifdef USE_VIVE
  svtkNew<svtkOpenVRRenderer> renderer;
  svtkNew<svtkOpenVRRenderWindow> renderWindow;
  svtkNew<svtkOpenVRRenderWindowInteractor> iren;
  svtkNew<svtkOpenVRCamera> cam;
#else
  svtkNew<svtkOpenGLRenderer> renderer;
  svtkNew<svtkWin32OpenGLRenderWindow> renderWindow;
  renderWindow->SetSize(1100, 1100);
  svtkNew<svtkWin32RenderWindowInteractor> iren;
  svtkNew<svtkOpenGLCamera> cam;
#endif

  renderWindow->SetMultiSamples(0);

  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  renderer->SetActiveCamera(cam);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  svtkNew<svtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(0.0, 1.0, 0.0);
  renderer->AddLight(light);

  svtkNew<svtkDICOMImageReader> reader;
  // reader->SetDirectoryName("C:/Users/Kenny/Documents/svtk/CTLung");
  // reader->SetDirectoryName("C:/Users/Kenny/Documents/svtk/Panc");
  reader->SetDirectoryName("C:/Users/Kenny/Documents/svtk/LIDC");
  reader->Update();
  reader->Print(cerr);

  svtkNew<svtkImageShrink3D> shrink;
  shrink->SetShrinkFactors(2, 2, 1);
  shrink->SetAveraging(1);
  shrink->SetInputConnection(reader->GetOutputPort());
  shrink->Update();
  shrink->GetOutput()->Print(cerr);

  svtkNew<svtkGPUVolumeRayCastMapper> mapper1;
  mapper1->SetInputConnection(reader->GetOutputPort());
  mapper1->SetInputConnection(shrink->GetOutputPort());
  mapper1->SetAutoAdjustSampleDistances(0);
  mapper1->SetSampleDistance(0.9);
  mapper1->UseJitteringOn();

  // gradientOpacity="4 0 1 255 1"
  // scalarOpacity="10 -3024 0 -1278.35 0 22.8277 0.428571 439.291 0.625 3071 0.616071"
  // specular="0" shade="1" ambient="0.2"
  // colorTransfer="20 -3024 0 0 0 -1278.35 0.54902 0.25098 0.14902 22.8277 0.882353 0.603922
  // 0.290196 439.291 1 0.937033 0.954531 3071 0.827451 0.658824 1" selectable="true" diffuse="1"
  // interpolation="1"/>

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(-250, 1.0, 0.6, 0.4);
  ctf->AddRGBPoint(40, 1.0, 0.6, 0.4);
  ctf->AddRGBPoint(450, 1.0, 1.0, 238 / 255.0);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 238 / 255.0);
  ctf->AddRGBPoint(3070, 0.2, 1.0, 0.3);

  svtkNew<svtkPiecewiseFunction> pwf;
  pwf->AddPoint(100, 0.0);
  pwf->AddPoint(500, 0.7);
  pwf->AddPoint(3071, 1.0);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(50, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty1;
  volumeProperty1->SetScalarOpacity(pwf);
  volumeProperty1->SetColor(ctf);
  // volumeProperty1->SetDisableGradientOpacity(1);
  volumeProperty1->SetGradientOpacity(gf);
  volumeProperty1->ShadeOn();
  volumeProperty1->SetAmbient(0.0);
  volumeProperty1->SetDiffuse(1.0);
  volumeProperty1->SetSpecular(0.0);
  volumeProperty1->SetInterpolationTypeToLinear();

  svtkNew<svtkVolume> volume1;
  volume1->SetMapper(mapper1);
  volume1->SetProperty(volumeProperty1);
  renderer->AddVolume(volume1);

  renderer->ResetCamera();
  // cam->SetViewAngle(110);
  renderWindow->Render();

#ifndef USE_VIVE
  // timing results
  // LINC dataset 512x512x133 - short
  // Shade, gf, jitter, sampleDist 0.5 = 90 FPS
  // NoShade, No gf, jitter, sampleDist 0.5 = 167 FPS
  // NoShade, No gf, No jitter, sampleDist 0.5 = 170 FPS
  // NoShade, No gf, No jitter, sampleDist 1.0 = 295 FPS
  // NoShade, No gf, jitter, sampleDist 1.0 = 285 FPS
  // NoShade, gf, jitter, sampleDist 1.0 = 190 FPS
  // Shade, gf, jitter, sampleDist 1.0 = 159 FPS
  // Shade, No gf, jitter, sampleDist 1.0 = 225 FPS
  // Shade, No gf, jitter, sampleDist 0.5 = 130 FPS

  // NoShade, No gf, No jitter, sampleDist auto = 225 FPS

  // rough results
  // Sample Dist = delta^0.8
  // jitter = 3% cost
  // shading = 20% cost
  // GF = 40% cost

  // Shade, gf, jitter, sampleDist 0.5 = 90 FPS
  // Shade, gf, jitter, sampleDist 0.5 (more opaque) = 109 FPS

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  int numFrames = 1000;
  for (int i = 0; i < numFrames; i++)
  {
    cam->Azimuth(1.0);
    renderWindow->Render();
  }
  timer->StopTimer();
  cerr << "FPS: " << floor(100 * numFrames / timer->GetElapsedTime()) / 100.0 << "\n";
#endif

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
