/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastLabelMap1Label.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Labeled data volume rendering with a single label
 */

#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkImageShiftScale.h>
#include <svtkMetaImageWriter.h>
#include <svtkObjectFactory.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSampleFunction.h>
#include <svtkSmartPointer.h>
#include <svtkSphere.h>
#include <svtkTesting.h>
#include <svtkVolume.h>
#include <svtkVolumeProperty.h>

namespace TestGPURayCastLabelMap1LabelNS
{
void CreateImageData(svtkImageData* a_image)
{
  // Create a spherical implicit function.
  svtkSmartPointer<svtkSphere> sphere = svtkSmartPointer<svtkSphere>::New();
  sphere->SetRadius(0.1);
  sphere->SetCenter(0.0, 0.0, 0.0);

  svtkSmartPointer<svtkSampleFunction> sampleFunc = svtkSmartPointer<svtkSampleFunction>::New();
  sampleFunc->SetImplicitFunction(sphere);
  sampleFunc->SetOutputScalarTypeToDouble();
  sampleFunc->SetSampleDimensions(127, 127, 127);
  sampleFunc->SetModelBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  sampleFunc->SetCapping(false);
  sampleFunc->SetComputeNormals(false);
  sampleFunc->SetScalarArrayName("values");
  sampleFunc->Update();

  svtkDataArray* scalars = sampleFunc->GetOutput()->GetPointData()->GetScalars("values");
  double scalarRange[2];
  scalars->GetRange(scalarRange);

  svtkSmartPointer<svtkImageShiftScale> shiftScale = svtkSmartPointer<svtkImageShiftScale>::New();
  shiftScale->SetInputConnection(sampleFunc->GetOutputPort());

  shiftScale->SetShift(-scalarRange[0]);
  double mag = scalarRange[1] - scalarRange[0];
  if (mag == 0.0)
  {
    mag = 1.0;
  }
  shiftScale->SetScale(255.0 / mag);
  shiftScale->SetOutputScalarTypeToShort();
  shiftScale->Update();

  a_image->DeepCopy(shiftScale->GetOutput());
}
} // end of namespace TestGPURayCastLabelMap1LabelNS

int TestGPURayCastLabelMap1Label(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create a sphere volume
  svtkNew<svtkImageData> imageData;
  TestGPURayCastLabelMap1LabelNS::CreateImageData(imageData);

  // prepare the rendering pipeline
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.3, 0.3);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // setup gpu rendering
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetBlendModeToComposite();
  mapper->SetInputData(imageData);
  mapper->SetAutoAdjustSampleDistances(true);

  // Main TF
  svtkNew<svtkPiecewiseFunction> opacityFunc;
  opacityFunc->AddPoint(0.0, 0.0);
  opacityFunc->AddPoint(80.0, 1.0);
  opacityFunc->AddPoint(80.1, 0.0);
  opacityFunc->AddPoint(255.0, 0.0);

  svtkNew<svtkColorTransferFunction> colorFunc;
  colorFunc->AddRGBPoint(0.0, 1.0, 0.0, 0.0);    // RED everywhere
  colorFunc->AddRGBPoint(40.0, 1.0, 0.0, 0.0);   // RED everywhere
  colorFunc->AddRGBPoint(255.0, 1.0, 0.0, 01.0); // RED everywhere

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetShade(true);
  volumeProperty->SetIndependentComponents(true);
  volumeProperty->SetColor(colorFunc);
  volumeProperty->SetScalarOpacity(opacityFunc);
  volumeProperty->SetInterpolationTypeToLinear();

  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  renderer->AddVolume(volume);
  renderer->ResetCamera();

  renWin->Render();

  // label map pipeline
  // prepare an empty label map of the same size;
  svtkNew<svtkImageData> labelMap;
  labelMap->SetOrigin(imageData->GetOrigin());
  labelMap->SetSpacing(imageData->GetSpacing());
  labelMap->SetDimensions(imageData->GetDimensions());
  labelMap->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);
  memset(labelMap->GetScalarPointer(), 1, sizeof(unsigned char) * labelMap->GetNumberOfPoints());

  svtkNew<svtkColorTransferFunction> labelMapColorFunc;
  labelMapColorFunc->AddRGBPoint(0.0, 0.0, 1.0, 0.0);   // GREEN everywhere
  labelMapColorFunc->AddRGBPoint(40.0, 0.0, 1.0, 0.0);  // GREEN everywhere
  labelMapColorFunc->AddRGBPoint(255.0, 0.0, 1.0, 0.0); // GREEN everywhere

  volumeProperty->SetLabelColor(1, labelMapColorFunc);
  volumeProperty->SetLabelScalarOpacity(1, opacityFunc);

  mapper->SetMaskInput(labelMap);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
