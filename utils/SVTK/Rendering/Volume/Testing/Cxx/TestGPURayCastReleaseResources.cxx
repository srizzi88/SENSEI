/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastReleaseResources.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is a test to make sure graphics resources are properly released and
// re-allocated when the context changes

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkCommand.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPlaneCollection.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastReleaseResources(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");

  reader->SetFileName(volumeFile);
  reader->Update();
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  delete[] volumeFile;

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  // Testing prefers image comparison with small images
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  // Intentional odd and NPOT  width/height
  renWin->SetSize(500, 401);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.1, 0.5, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 0.5, 0.1);

  // Setup volume actor
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->GetActiveCamera()->Azimuth(-40);
  ren->ResetCamera();
  renWin->Render();

  // Delete the old render window to release graphics resources
  renWin->Delete();

  svtkNew<svtkRenderWindow> renWin2;
  renWin2->SetSize(300, 401);
  renWin2->AddRenderer(ren);
  renWin2->Render();

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin2);

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin2);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
