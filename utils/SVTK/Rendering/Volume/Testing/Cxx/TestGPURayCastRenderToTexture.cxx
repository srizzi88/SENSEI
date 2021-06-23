/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastRenderToTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test the GPU volume mapper low level API to render color and depth buffers to
// the frame buffer.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastRenderToTexture(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkNew<svtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange(1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);

  delete[] fname;

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->RenderToImageOn();

  svtkNew<svtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(900.0, 198 / 255.0, 134 / 255.0, 66 / 255.0);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(0, 0.0);
  scalarOpacity->AddPoint(70, 0.0);
  scalarOpacity->AddPoint(449, 0.0);
  scalarOpacity->AddPoint(900, 0.15);
  scalarOpacity->AddPoint(1120, 0.25);
  scalarOpacity->AddPoint(1404, 0.35);
  scalarOpacity->AddPoint(4095, 0.5);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetColor(colorFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  // Setup volume actor
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Testing prefers image comparison with small images
  svtkNew<svtkRenderWindow> renWin;
  // Intentional odd and NPOT  width/height
  renWin->SetSize(401, 399);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->AddVolume(volume);
  ren->GetActiveCamera()->Azimuth(90);
  ren->GetActiveCamera()->Roll(90);
  ren->GetActiveCamera()->Azimuth(-90);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.8);
  renWin->Render();

  svtkNew<svtkImageData> im;

  // Get color texture as image
  volumeMapper->GetColorImage(im);

  ren->RemoveVolume(volume);

  svtkNew<svtkImageActor> ia;
  ia->GetMapper()->SetInputData(im);
  ren->AddActor(ia);
  ren->GetActiveCamera()->SetPosition(0, 0, -1);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 1);
  ren->GetActiveCamera()->SetViewUp(0, 1, 0);
  ren->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
