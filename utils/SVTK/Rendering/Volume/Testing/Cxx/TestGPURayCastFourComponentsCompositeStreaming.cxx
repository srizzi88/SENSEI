/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastFourComponentsCompositeStreaming.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test volume renders the vase dataset with 4 dependent components the
// composite method with no shading.

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkImageMagnify.h"
#include "svtkImageShiftScale.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTransform.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

int TestGPURayCastFourComponentsCompositeStreaming(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_4comp.vti");

  svtkXMLImageDataReader* reader = svtkXMLImageDataReader::New();
  reader->SetFileName(cfname);
  delete[] cfname;

  svtkImageMagnify* mag = svtkImageMagnify::New();
  mag->SetInputConnection(reader->GetOutputPort());
  mag->SetMagnificationFactors(5, 1, 1);
  mag->SetInterpolate(1);

  int dims[3];
  mag->Update();
  mag->GetOutput()->GetDimensions(dims);
  unsigned long sizekb = mag->GetOutput()->GetActualMemorySize();
  cout << "Memory usage for the ImageData=" << (sizekb / 1024) << "Mb" << endl;
  cout << "Dims of the ImageData=" << dims[0] << "x" << dims[1] << "x" << dims[2] << "="
       << (dims[0] * dims[1] * dims[2]) / 1024 / 1024 << "Mb" << endl;

  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(301, 300);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  renWin->Render();

  svtkGPUVolumeRayCastMapper* volumeMapper;
  svtkVolumeProperty* volumeProperty;
  svtkVolume* volume;

  volumeMapper = svtkGPUVolumeRayCastMapper::New();
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputConnection(mag->GetOutputPort());

  svtkPiecewiseFunction* opacity = svtkPiecewiseFunction::New();
  opacity->AddPoint(0, 0);
  opacity->AddPoint(255, 1);

  volumeProperty = svtkVolumeProperty::New();
  volumeProperty->IndependentComponentsOff();
  volumeProperty->ShadeOff();
  volumeProperty->SetScalarOpacity(opacity);

  volume = svtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  int valid = volumeMapper->IsRenderSupported(renWin, volumeProperty);

  int retVal;
  if (valid)
  {
    iren->Initialize();
    ren1->SetBackground(0.1, 0.4, 0.2);
    ren1->ResetCamera();
    renWin->Render();

    retVal = svtkTesting::Test(argc, argv, renWin, 75);
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

  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();

  opacity->Delete();
  mag->Delete();

  reader->Delete();

  if ((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
