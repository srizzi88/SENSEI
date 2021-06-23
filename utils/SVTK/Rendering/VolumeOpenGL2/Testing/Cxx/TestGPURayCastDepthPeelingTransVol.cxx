/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastDepthPeelingTransVol.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 *  Tests depth peeling pass with volume rendering.
 */

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderer.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkTestingObjectFactory.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastDepthPeelingTransVol(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> ren;
  renWin->Render(); // Create the context
  renWin->AddRenderer(ren);
  svtkOpenGLRenderer* oglRen = svtkOpenGLRenderer::SafeDownCast(ren);
  assert(oglRen); // This test should only be enabled for OGL2 backend.
  // This will print details about why depth peeling is unsupported:
  oglRen->SetDebug(1);
  bool supported = oglRen->IsDualDepthPeelingSupported();
  oglRen->SetDebug(0);
  if (!supported)
  {
    std::cerr << "Skipping test; volume peeling not supported.\n";
    return SVTK_SKIP_RETURN_CODE;
  }

  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  renWin->SetMultiSamples(0);
  renWin->SetSize(800, 400);

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.06);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.9, 0.4, 0.9);

  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  int dims[3];
  double spacing[3], center[3], origin[3];
  reader->Update();
  svtkSmartPointer<svtkImageData> im = reader->GetOutput();
  im->GetDimensions(dims);
  im->GetOrigin(origin);
  im->GetSpacing(spacing);

  // sphere 1
  center[0] = origin[0] + spacing[0] * dims[0] / 2.0;
  center[1] = origin[1] + spacing[1] * dims[1] / 2.0;
  center[2] = origin[2] + spacing[2] * dims[2] / 2.0;

  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(center);
  sphereSource->SetRadius(dims[1] / 3.0);
  svtkNew<svtkActor> sphereActor;
  svtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.5, 0.9, 0.7);
  sphereProperty->SetOpacity(0.3);
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  // sphere 2
  center[0] += 15.0;
  center[1] += 15.0;
  center[2] += 15.0;

  svtkNew<svtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(center);
  sphereSource2->SetRadius(dims[1] / 3.0);
  svtkNew<svtkActor> sphereActor2;
  sphereProperty = sphereActor2->GetProperty();
  sphereProperty->SetColor(0.9, 0.4, 0.1);
  sphereProperty->SetOpacity(0.3);
  svtkNew<svtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2);

  // sphere 3 (transparent)
  svtkNew<svtkActor> sphereActorTransp;
  svtkProperty* sphereProperty3 = sphereActorTransp->GetProperty();
  sphereProperty3->SetColor(0.0, 1.0, 0.0);
  sphereProperty3->SetOpacity(0.0);
  svtkNew<svtkPolyDataMapper> sphereMapperTransp;
  sphereMapperTransp->SetInputConnection(sphereSource->GetOutputPort());
  sphereActorTransp->SetMapper(sphereMapperTransp);

  // Translucent spheres
  ren->SetBackground(1.0, 1.0, 1.0);
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);

  ren->AddVolume(volume);
  ren->AddActor(sphereActor);
  ren->AddActor(sphereActor2);

  ren->SetUseDepthPeeling(1);
  ren->SetOcclusionRatio(0.0);
  ren->SetMaximumNumberOfPeels(17);
  ren->SetUseDepthPeelingForVolumes(true);

  // Fully trasnparent sphere
  svtkNew<svtkRenderer> ren2;
  ren2->SetBackground(1.0, 1.0, 1.0);
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  ren2->SetActiveCamera(ren->GetActiveCamera());

  ren2->AddVolume(volume);
  ren2->AddActor(sphereActorTransp);

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(17);
  ren2->SetUseDepthPeelingForVolumes(true);
  renWin->AddRenderer(ren2);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-55);
  ren->GetActiveCamera()->Elevation(35);
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
