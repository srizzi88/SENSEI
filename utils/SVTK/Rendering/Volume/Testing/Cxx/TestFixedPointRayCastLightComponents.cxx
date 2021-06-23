/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMIPToComposite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test covers svtkFixedPointVolumeRayCastMapper with a light where
// diffuse and specular components are different.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method. The diffuse light component is gray, the
// light specular component is blue.

#include "svtkSampleFunction.h"
#include "svtkSphere.h"

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataArray.h"
#include "svtkFixedPointVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolumeProperty.h"
#include <cassert>

int TestFixedPointRayCastLightComponents(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create a spherical implicit function.
  svtkSphere* shape = svtkSphere::New();
  shape->SetRadius(0.1);
  shape->SetCenter(0.0, 0.0, 0.0);

  svtkSampleFunction* source = svtkSampleFunction::New();
  source->SetImplicitFunction(shape);
  shape->Delete();
  source->SetOutputScalarTypeToDouble();
  source->SetSampleDimensions(127, 127, 127); // intentional NPOT dimensions.
  source->SetModelBounds(-100.0, 100.0, -100.0, 100.0, -100.0, 100.0);
  source->SetCapping(false);
  source->SetComputeNormals(false);
  source->SetScalarArrayName("values");

  source->Update();

  svtkDataArray* a = source->GetOutput()->GetPointData()->GetScalars("values");
  double range[2];
  a->GetRange(range);

  svtkImageShiftScale* t = svtkImageShiftScale::New();
  t->SetInputConnection(source->GetOutputPort());
  source->Delete();
  t->SetShift(-range[0]);
  double magnitude = range[1] - range[0];
  if (magnitude == 0.0)
  {
    magnitude = 1.0;
  }
  t->SetScale(255.0 / magnitude);
  t->SetOutputScalarTypeToUnsignedChar();

  t->Update();

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren1 = svtkRenderer::New();
  ren1->SetBackground(0.1, 0.4, 0.2);

  renWin->AddRenderer(ren1);
  ren1->Delete();
  renWin->SetSize(301, 300); // intentional odd and NPOT width/height

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkLightCollection* lights = ren1->GetLights();
  assert("check: lights_empty" && lights->GetNumberOfItems() == 0);
  svtkLight* light = svtkLight::New();
  light->SetAmbientColor(0.0, 0.0, 0.0);
  light->SetDiffuseColor(0.5, 0.5, 0.5);
  light->SetSpecularColor(0.0, 0.0, 1.0);
  light->SetIntensity(1.0);
  // positional light is not supported by svtkFixedPointVolumeRayCastMapper
  light->SetLightTypeToHeadlight();
  lights->AddItem(light);
  light->Delete();

  svtkVolumeProperty* volumeProperty;
  svtkVolume* volume;

  svtkFixedPointVolumeRayCastMapper* volumeMapper = svtkFixedPointVolumeRayCastMapper::New();
  volumeMapper->SetSampleDistance(1.0);
  volumeMapper->SetNumberOfThreads(1);

  volumeMapper->SetInputConnection(t->GetOutputPort());

  volumeProperty = svtkVolumeProperty::New();
  volumeMapper->SetBlendModeToComposite();
  volumeProperty->ShadeOn();
  volumeProperty->SetSpecularPower(128.0);
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  svtkPiecewiseFunction* compositeOpacity = svtkPiecewiseFunction::New();
  compositeOpacity->AddPoint(0.0, 1.0);   // 1.0
  compositeOpacity->AddPoint(80.0, 1.0);  // 1.0
  compositeOpacity->AddPoint(80.1, 0.0);  // 0.0
  compositeOpacity->AddPoint(255.0, 0.0); // 0.0
  volumeProperty->SetScalarOpacity(compositeOpacity);

  svtkColorTransferFunction* color = svtkColorTransferFunction::New();
  color->AddRGBPoint(0.0, 1.0, 1.0, 1.0);   // blue
  color->AddRGBPoint(40.0, 1.0, 1.0, 1.0);  // red
  color->AddRGBPoint(255.0, 1.0, 1.0, 1.0); // white
  volumeProperty->SetColor(color);
  color->Delete();

  volume = svtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  int retVal;

  ren1->ResetCamera();
  renWin->Render();

  retVal = svtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();
  iren->Delete();
  t->Delete();
  compositeOpacity->Delete();

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
