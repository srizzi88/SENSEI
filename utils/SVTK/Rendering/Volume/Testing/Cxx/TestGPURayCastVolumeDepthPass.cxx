/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeUpdate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test volume tests whether updating the volume MTime updates the ,
// geometry in the volume mapper.

#include <svtkColorTransferFunction.h>
#include <svtkContourValues.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkTesting.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastVolumeDepthPass(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  // This is the feature we are testing
  volumeMapper->UseDepthPassOn();
  volumeMapper->GetDepthPassContourValues()->SetValue(0, 50.0);

  // Set other parameters
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render(); // make sure we have an OpenGL context.

  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->AddRenderer(ren);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
  volumeProperty->SetColor(colorTransferFunction);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddVolume(volume);
  ren->ResetCamera();

  int valid = volumeMapper->IsRenderSupported(renWin, volumeProperty);

  int retVal;
  if (valid)
  {
    renWin->Render();

    iren->Initialize();

    retVal = svtkRegressionTestImage(renWin);
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    return !retVal;
  }
  else
  {
    retVal = svtkTesting::PASSED;
    cout << "Required extensions not supported" << endl;
  }

  return !retVal;
}
