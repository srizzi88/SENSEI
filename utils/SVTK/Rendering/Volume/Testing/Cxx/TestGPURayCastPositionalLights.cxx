/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastPositionalLights.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test volume renders a synthetic dataset with four different
// positional lights in the scene.

#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageData.h>
#include <svtkLight.h>
#include <svtkNew.h>
#include <svtkPiecewiseFunction.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTestUtilities.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

#include <svtkActor.h>
#include <svtkContourFilter.h>
#include <svtkLightActor.h>
#include <svtkPolyDataMapper.h>

int TestGPURayCastPositionalLights(int argc, char* argv[])
{
  double scalarRange[2];

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  delete[] volumeFile;

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.1);

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.0, 0.0, 0.4);
  ren->AutomaticLightCreationOff();
  ren->RemoveAllLights();

  svtkNew<svtkLight> light1;
  light1->SetLightTypeToSceneLight();
  light1->SetPositional(true);
  light1->SetDiffuseColor(1, 0, 0);
  light1->SetAmbientColor(0, 0, 0);
  light1->SetSpecularColor(1, 1, 1);
  light1->SetConeAngle(60);
  light1->SetPosition(0.0, 0.0, 100.0);
  light1->SetFocalPoint(0.0, 0.0, 0.0);
  //  light1->SetColor(1,0,0);
  //  light1->SetPosition(40,40,301);
  //  light1->SetPosition(-57, -50, -360);

  svtkNew<svtkLightActor> lightActor;
  lightActor->SetLight(light1);
  ren->AddViewProp(lightActor);
  svtkNew<svtkLight> light2;
  svtkNew<svtkLight> light3;
  svtkNew<svtkLight> light4;

  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
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
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);

  svtkNew<svtkPolyDataMapper> pm;
  svtkNew<svtkActor> ac;
  svtkNew<svtkContourFilter> cf;
  ac->SetMapper(pm);
  pm->SetInputConnection(cf->GetOutputPort());
  pm->SetScalarVisibility(0);
  cf->SetValue(0, 60.0);
  cf->SetInputConnection(reader->GetOutputPort());
  ac->SetPosition(-89.0, 0.0, 0.0);
  volume->SetPosition(-30.0, 0.0, 0.0);
  ren->AddActor(ac);
  svtkNew<svtkActor> ac1;
  ac1->SetMapper(pm);
  ac1->SetPosition(0, 0, 0);
  ren->SetTwoSidedLighting(0);

  ren->AddLight(light1);
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
