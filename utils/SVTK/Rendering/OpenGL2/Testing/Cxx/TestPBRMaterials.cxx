/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBRMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the PBR Interpolation shading
// It renders spheres with different materials using a skybox as image based lighting

#include "svtkActor.h"
#include "svtkActorCollection.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkImageData.h"
#include "svtkImageFlip.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLSkybox.h"
#include "svtkOpenGLTexture.h"
#include "svtkPBRIrradianceTexture.h"
#include "svtkPBRLUTTexture.h"
#include "svtkPBRPrefilterTexture.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

//----------------------------------------------------------------------------
int TestPBRMaterials(int argc, char* argv[])
{
  svtkNew<svtkOpenGLRenderer> renderer;

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkOpenGLSkybox> skybox;

  svtkSmartPointer<svtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  svtkSmartPointer<svtkPBRPrefilterTexture> prefilter = renderer->GetEnvMapPrefiltered();
  prefilter->SetPrefilterSamples(64);
  prefilter->SetPrefilterSize(64);

  svtkNew<svtkOpenGLTexture> textureCubemap;
  textureCubemap->CubeMapOn();

  std::string pathSkybox[6] = { "Data/skybox/posx.jpg", "Data/skybox/negx.jpg",
    "Data/skybox/posy.jpg", "Data/skybox/negy.jpg", "Data/skybox/posz.jpg",
    "Data/skybox/negz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkJPEGReader> jpg;
    char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, pathSkybox[i].c_str());
    jpg->SetFileName(fname);
    delete[] fname;
    svtkNew<svtkImageFlip> flip;
    flip->SetInputConnection(jpg->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    textureCubemap->SetInputConnection(i, flip->GetOutputPort());
  }

  renderer->SetEnvironmentTexture(textureCubemap, true);
  renderer->UseImageBasedLightingOn();

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);

  svtkNew<svtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkActor> actorSphere;
    actorSphere->SetPosition(i, 1.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.72, 0.45, 0.2);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkActor> actorSphere;
    actorSphere->SetPosition(i, 2.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkActor> actorSphere;
    actorSphere->SetPosition(i, 3.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    svtkNew<svtkActor> actorSphere;
    actorSphere->SetPosition(i, 4.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  skybox->SetTexture(textureCubemap);
  renderer->AddActor(skybox);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
