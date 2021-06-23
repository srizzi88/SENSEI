/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageChangeInformation.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkNew.h>
#include <svtkOSPRayPass.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

// TODO Test geometry compositing with gl2. The only significant
// differences appear to be along the box frame (outlineActor).

int TestGPURayCastVolumeScale(int argc, char* argv[])
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
  double scalarRange[2];

  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);

  svtkSmartPointer<svtkImageChangeInformation> changeInformation =
    svtkSmartPointer<svtkImageChangeInformation>::New();
  changeInformation->SetInputConnection(reader->GetOutputPort());
  changeInformation->SetOutputSpacing(1, 2, 3);
  changeInformation->SetOutputOrigin(10, 20, 30);
  changeInformation->Update();
  volumeMapper->SetInputConnection(changeInformation->GetOutputPort());

  // Add outline filter
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(changeInformation->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  // OsprayPolyDataMapperNode requires transfer function spec.
  svtkProperty* property = outlineActor->GetProperty();
  property->SetColor(1.0, 1.0, 1.0);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);
  ren->SetBackground(0.2, 0.2, 0.5);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

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
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->AddActor(outlineActor);

  // Attach OSPRay render pass
  svtkNew<svtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren->SetPass(osprayPass);
  }

  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
