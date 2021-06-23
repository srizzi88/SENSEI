/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test is intended to benchmark render times for the svtkGPURayCastMapper

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

//----------------------------------------------------------------------------
int TestGPURayCastMapperBenchmark(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  svtkNew<svtkVolumeProperty> volumeProperty;
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  svtkNew<svtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(900, 900);
  renderWindow->Render(); // make sure we have an OpenGL context.

  svtkNew<svtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);
  int retVal;
  if (valid)
  {

    svtkNew<svtkTimerLog> timer;
    timer->StartTimer();
    renderWindow->Render();
    timer->StopTimer();
    double firstRender = timer->GetElapsedTime();
    cerr << "First Render Time: " << firstRender << endl;

    int numRenders = 20;
    for (int i = 0; i < numRenders; ++i)
    {
      renderer->GetActiveCamera()->Azimuth(1);
      renderer->GetActiveCamera()->Elevation(1);
      renderWindow->Render();
    }

    timer->StartTimer();
    numRenders = 100;
    for (int i = 0; i < numRenders; ++i)
    {
      renderer->GetActiveCamera()->Azimuth(1);
      renderer->GetActiveCamera()->Elevation(1);
      renderer->GetActiveCamera()->OrthogonalizeViewUp();
      renderWindow->Render();
    }
    timer->StopTimer();
    double elapsed = timer->GetElapsedTime();
    cerr << "Interactive Render Time: " << elapsed / numRenders << endl;

    renderer->GetActiveCamera()->SetPosition(0, 0, 1);
    renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    renderer->ResetCamera();

    renderWindow->SetSize(300, 300);
    renderWindow->Render();

    iren->Initialize();

    retVal = svtkRegressionTestImage(renderWindow);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = svtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
