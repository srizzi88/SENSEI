/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBRMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the PBR Interpolation shading
// It renders a cube with custom texture mapping

#include "svtkActor.h"
#include "svtkActorCollection.h"
#include "svtkCamera.h"
#include "svtkCubeSource.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkImageData.h"
#include "svtkImageFlip.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkJPEGReader.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLSkybox.h"
#include "svtkOpenGLTexture.h"
#include "svtkPBRIrradianceTexture.h"
#include "svtkPBRLUTTexture.h"
#include "svtkPBRPrefilterTexture.h"
#include "svtkPNGReader.h"
#include "svtkPolyDataTangents.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtkTriangleFilter.h"

//----------------------------------------------------------------------------
int TestPBRMapping(int argc, char* argv[])
{
  svtkNew<svtkOpenGLRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  svtkNew<svtkLight> light;
  light->SetPosition(2.0, 0.0, 2.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);

  renderer->AddLight(light);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  svtkSmartPointer<svtkPBRPrefilterTexture> prefilter = renderer->GetEnvMapPrefiltered();
  prefilter->SetPrefilterSamples(64);
  prefilter->SetPrefilterSize(64);

  svtkNew<svtkOpenGLTexture> textureCubemap;
  textureCubemap->CubeMapOn();
  textureCubemap->UseSRGBColorSpaceOn();

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

  renderer->SetEnvironmentTexture(textureCubemap);
  renderer->UseImageBasedLightingOn();

  svtkNew<svtkCubeSource> cube;

  svtkNew<svtkTriangleFilter> triangulation;
  triangulation->SetInputConnection(cube->GetOutputPort());

  svtkNew<svtkPolyDataTangents> tangents;
  tangents->SetInputConnection(triangulation->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  svtkNew<svtkPNGReader> materialReader;
  char* matname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/svtk_Material.png");
  materialReader->SetFileName(matname);
  delete[] matname;

  svtkNew<svtkTexture> material;
  material->InterpolateOn();
  material->SetInputConnection(materialReader->GetOutputPort());

  svtkNew<svtkPNGReader> albedoReader;
  char* colname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/svtk_Base_Color.png");
  albedoReader->SetFileName(colname);
  delete[] colname;

  svtkNew<svtkTexture> albedo;
  albedo->UseSRGBColorSpaceOn();
  albedo->InterpolateOn();
  albedo->SetInputConnection(albedoReader->GetOutputPort());

  svtkNew<svtkPNGReader> normalReader;
  char* normname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/svtk_Normal.png");
  normalReader->SetFileName(normname);
  delete[] normname;

  svtkNew<svtkTexture> normal;
  normal->InterpolateOn();
  normal->SetInputConnection(normalReader->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetOrientation(0.0, 25.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  // set metallic and roughness to 1.0 as they act as multipliers with texture value
  actor->GetProperty()->SetMetallic(1.0);
  actor->GetProperty()->SetRoughness(1.0);

  actor->GetProperty()->SetBaseColorTexture(albedo);
  actor->GetProperty()->SetORMTexture(material);
  actor->GetProperty()->SetNormalTexture(normal);

  renderer->AddActor(actor);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
