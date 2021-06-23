/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPlaneSource.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

#include "svtkLight.h"

//----------------------------------------------------------------------------
int TestSRGB(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 400);
  // renderWindow->SetUseSRGBColorSpace(true); // not supported on all hardware
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.Get());

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/skybox/posz.jpg");
  svtkNew<svtkJPEGReader> imgReader;
  imgReader->SetFileName(fileName);

  delete[] fileName;

  svtkNew<svtkPlaneSource> plane;

  for (int i = 0; i < 2; i++)
  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(i == 0 ? 0.0 : 0.5, 0.0, i == 0 ? 0.5 : 1.0, 1.0);
    renderer->SetBackground(0.3, 0.3, 0.3);
    renderWindow->AddRenderer(renderer.Get());

    {
      svtkNew<svtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(-1.73, -1.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(90);
      light->SetAttenuationValues(0, 1.0, 0);
      light->SetColor(4, 0, 0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }
    {
      svtkNew<svtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(1.73, -1.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(90);
      light->SetAttenuationValues(0, 0, 1.0);
      light->SetColor(0, 6, 0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }
    {
      svtkNew<svtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(0.0, 2.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(50);
      light->SetColor(0, 0, 4);
      light->SetAttenuationValues(1.0, 0.0, 0.0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }

    svtkNew<svtkTexture> texture;
    texture->InterpolateOn();
    texture->RepeatOff();
    texture->EdgeClampOn();
    texture->SetUseSRGBColorSpace(i == 0);
    texture->SetInputConnection(imgReader->GetOutputPort(0));

    svtkNew<svtkOpenGLPolyDataMapper> mapper;
    mapper->SetInputConnection(plane->GetOutputPort());

    svtkNew<svtkActor> actor;
    actor->SetPosition(0, 0, 0);
    actor->SetScale(6.0, 6.0, 6.0);
    actor->GetProperty()->SetSpecular(0.2);
    actor->GetProperty()->SetSpecularPower(20);
    actor->GetProperty()->SetDiffuse(0.9);
    actor->GetProperty()->SetAmbient(0.2);
    // actor->GetProperty()->SetDiffuse(0.0);
    // actor->GetProperty()->SetAmbient(1.0);
    renderer->AddActor(actor.Get());
    actor->SetTexture(texture.Get());
    actor->SetMapper(mapper.Get());

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.3);
    renderer->ResetCameraClippingRange();
  }

  renderWindow->Render();
  cout << "Render window sRGB status: "
       << static_cast<svtkOpenGLRenderWindow*>(renderWindow.Get())->GetUsingSRGBColorSpace() << "\n";
  int retVal = svtkRegressionTestImage(renderWindow.Get());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
