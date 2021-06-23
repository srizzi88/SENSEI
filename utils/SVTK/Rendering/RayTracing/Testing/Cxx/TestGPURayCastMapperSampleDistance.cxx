/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test checks the effects of changing sample distance on the GPURayCast
// volume mapper

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

#include "svtkOSPRayPass.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestGPURayCastMapperSampleDistance(int argc, char* argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(20);

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
  renderWindow->SetSize(300, 300);
  renderWindow->Render(); // make sure we have an OpenGL context.

  svtkNew<svtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  // Attach OSPRay render pass
  svtkNew<svtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    renderer->SetPass(osprayPass);
  }

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);

  int retVal;
  if (valid)
  {
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
