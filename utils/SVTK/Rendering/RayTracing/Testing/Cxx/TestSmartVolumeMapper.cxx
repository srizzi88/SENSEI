/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the smart volume mapper and composite method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include <svtkCamera.h>
#include <svtkClipPolyData.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOSPRayPass.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSmartVolumeMapper.h>
#include <svtkStructuredPointsReader.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

#include <svtkAutoInit.h>
SVTK_MODULE_INIT(svtkRenderingRayTracing);

int TestSmartVolumeMapper(int argc, char* argv[])
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

  svtkNew<svtkActor> dssActor;
  svtkNew<svtkPolyDataMapper> dssMapper;
  svtkNew<svtkSmartVolumeMapper> volumeMapper;
  if (useOSP)
  {
    volumeMapper->SetRequestedRenderModeToOSPRay();
  }

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetSampleDistance(0.01);

  // Put inside an open box to evaluate composite order
  svtkNew<svtkDataSetSurfaceFilter> dssFilter;
  dssFilter->SetInputConnection(reader->GetOutputPort());
  svtkNew<svtkClipPolyData> clip;
  svtkNew<svtkPlane> plane;
  plane->SetOrigin(0, 50, 0);
  plane->SetNormal(0, -1, 0);
  clip->SetInputConnection(dssFilter->GetOutputPort());
  clip->SetClipFunction(plane);
  dssMapper->SetInputConnection(clip->GetOutputPort());
  dssMapper->ScalarVisibilityOff();
  dssActor->SetMapper(dssMapper);
  svtkProperty* property = dssActor->GetProperty();
  property->SetDiffuseColor(0.5, 0.5, 0.5);

  reader->Update();
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(1);
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  //  svtkNew<svtkInteractorStyleTrackballCamera> style;
  //  iren->SetInteractorStyle(style);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.1);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.8, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.0, 0.8, 0.1);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->AddActor(dssActor);
  renWin->Render();
  ren->ResetCamera();

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = svtkRegressionTestImageThreshold(renWin, 50.0);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
