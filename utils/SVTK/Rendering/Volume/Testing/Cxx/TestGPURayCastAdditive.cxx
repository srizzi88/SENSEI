/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastAdditive.cxx

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

#include "svtkSampleFunction.h"
#include "svtkSphere.h"

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkDataArray.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastAdditive(int argc, char* argv[])
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
  source->SetModelBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
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

  // intentional odd and NPOT width/height
  renWin->SetSize(301, 300);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  // make sure we have an OpenGL context.
  renWin->Render();

  svtkGPUVolumeRayCastMapper* volumeMapper;
  svtkVolumeProperty* volumeProperty;
  svtkVolume* volume;

  volumeMapper = svtkGPUVolumeRayCastMapper::New();
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.2);
  volumeMapper->SetBlendModeToComposite(); // composite first
  volumeMapper->SetInputConnection(t->GetOutputPort());

  volumeProperty = svtkVolumeProperty::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  svtkPiecewiseFunction* additiveOpacity = svtkPiecewiseFunction::New();
  additiveOpacity->AddPoint(0.0, 0.0);
  additiveOpacity->AddPoint(200.0, 0.5);
  additiveOpacity->AddPoint(200.1, 1.0);
  additiveOpacity->AddPoint(255.0, 1.0);

  svtkPiecewiseFunction* compositeOpacity = svtkPiecewiseFunction::New();
  compositeOpacity->AddPoint(0.0, 0.0);
  compositeOpacity->AddPoint(80.0, 1.0);
  compositeOpacity->AddPoint(80.1, 0.0);
  compositeOpacity->AddPoint(255.0, 0.0);
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  svtkColorTransferFunction* color = svtkColorTransferFunction::New();
  color->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(40.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
  volumeProperty->SetColor(color);
  color->Delete();

  volume = svtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  int valid = volumeMapper->IsRenderSupported(renWin, volumeProperty);

  int retVal;
  if (valid)
  {
    ren1->ResetCamera();

    // Render composite.
    renWin->Render();

    // Switch to Additive
    volumeMapper->SetBlendModeToAdditive();
    volumeProperty->SetScalarOpacity(additiveOpacity);
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

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();
  iren->Delete();
  t->Delete();
  additiveOpacity->Delete();
  compositeOpacity->Delete();

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
