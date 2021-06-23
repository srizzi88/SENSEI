/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumePolyData.cxx

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
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPlane.h>
#include <svtkPlaneCollection.h>
#include <svtkPointData.h>
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

/// Tests volume clipping when intermixed with geometry.
int TestGPURayCastClippingPolyData(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  delete[] volumeFile;

  /// Outline filter
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);
  ren->SetBackground(0.2, 0.2, 0.5);

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);

  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  /// Sphere
  int dims[3];
  double spacing[3], sphereCenter[3], origin[3];
  reader->Update();
  svtkSmartPointer<svtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  sphereCenter[0] = origin[0] + spacing[0] * dims[0] / 2.5;
  sphereCenter[1] = origin[1] + spacing[1] * dims[1] / 2.5;
  sphereCenter[2] = origin[2] + spacing[2] * dims[2] / 2.775;

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(sphereCenter);
  sphereSource->SetRadius(dims[1] / 4.0);
  sphereSource->SetPhiResolution(40);
  sphereSource->SetThetaResolution(40);
  svtkNew<svtkPolyDataMapper> sphereMapper;
  svtkNew<svtkActor> sphereActor;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  ren->AddViewProp(volume);
  ren->AddActor(outlineActor);
  ren->AddActor(sphereActor);

  /// Clipping planes
  svtkNew<svtkPlane> clipPlane1;
  clipPlane1->SetOrigin(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
  clipPlane1->SetNormal(1.0, 0.0, 0.0);
  svtkNew<svtkPlane> clipPlane2;
  clipPlane2->SetOrigin(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  svtkNew<svtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-30);
  ren->GetActiveCamera()->Elevation(25);
  ren->GetActiveCamera()->OrthogonalizeViewUp();
  renWin->Render();

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
