/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastChangedArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Designed to test paraview/paraview#19012: when the array to volume render
 * with is changed, the volume mapper must update correctly.
 */

#include <svtkArrayCalculator.h>
#include <svtkColorTransferFunction.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderer.h>
#include <svtkPiecewiseFunction.h>
#include <svtkProperty.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTestUtilities.h>
#include <svtkTesting.h>
#include <svtkTestingObjectFactory.h>
#include <svtkVolumeProperty.h>

int TestGPURayCastChangedArray(int argc, char* argv[])
{
  svtkNew<svtkRTAnalyticSource> rtSource;
  rtSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  svtkNew<svtkArrayCalculator> calculator;
  calculator->SetInputConnection(rtSource->GetOutputPort());
  calculator->AddScalarArrayName("RTData");
  calculator->SetResultArrayName("sin_RTData");
  calculator->SetFunction("100*sin(RTData)");

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(calculator->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("sin_RTData");

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(250.0, 1.0, 1.0, 1.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(0, 0.0);
  scalarOpacity->AddPoint(250, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  svtkNew<svtkRenderer> renderer;
  renderer->AddVolume(volume);

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  iren->SetInteractorStyle(style);

  // render using sin_RTData.
  mapper->SelectScalarArray("sin_RTData");
  renderWindow->Render();
  renderer->ResetCamera();

  // change array and re-render.
  mapper->SelectScalarArray("RTData");
  renderWindow->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
