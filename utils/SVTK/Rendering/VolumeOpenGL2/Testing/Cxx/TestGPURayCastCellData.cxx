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
/**
 * This test volume tests whether updating the volume MTime updates the ,
 * geometry in the volume mapper.
 *
 * Added renderer to expand coverage for svtkDualDepthPeelingPass.
 */

#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkNew.h>
#include <svtkOpenGLRenderer.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointDataToCellData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRTAnalyticSource.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkTesting.h>
#include <svtkTestingObjectFactory.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

int TestGPURayCastCellData(int argc, char* argv[])
{
  // Volume peeling is only supported through the dual depth peeling algorithm.
  // If the current system only supports the legacy peeler, skip this test:
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->Render(); // Create the context

  svtkNew<svtkRenderer> ren;
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

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  double scalarRange[2];

  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;

  svtkNew<svtkXMLImageDataReader> reader;
  char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  svtkNew<svtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetInputConnection(pointToCell->GetOutputPort());

  // Add outline filter
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(pointToCell->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1);
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite();

  renWin->SetMultiSamples(0);
  renWin->SetSize(800, 400);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  // Initialize OpenGL context
  renWin->Render();

  // Renderer without translucent geometry
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);
  ren->SetBackground(0.2, 0.2, 0.5);

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
  ren->AddActor(outlineActor);

  // Renderer with translucent geometry
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetCenter(80.0, 60.0, 30.0);
  sphereSource->SetRadius(30.0);

  svtkNew<svtkActor> sphereActor;
  svtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(1., 0.9, 1);
  sphereProperty->SetOpacity(0.4);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  svtkNew<svtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  ren2->SetBackground(0.2, 0.2, 0.5);
  ren2->SetActiveCamera(ren->GetActiveCamera());

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(5);
  ren2->SetUseDepthPeelingForVolumes(true);

  ren2->AddVolume(volume);
  ren2->AddActor(outlineActor);
  ren2->AddActor(sphereActor);
  renWin->AddRenderer(ren2);

  ren->ResetCamera();

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
