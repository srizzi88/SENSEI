/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNormalMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the normal mapping feature
// Texture credits:
// Julian Herzog, CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
// The image has been cropped and resized

#include "svtkActor.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkPNGReader.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataTangents.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtkTriangleFilter.h"

//----------------------------------------------------------------------------
int TestNormalMapping(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  svtkNew<svtkLight> light;
  light->SetPosition(0.5, 0.5, 1.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);

  renderer->AddLight(light);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkPlaneSource> plane;

  svtkNew<svtkTriangleFilter> triangulation;
  triangulation->SetInputConnection(plane->GetOutputPort());

  svtkNew<svtkPolyDataTangents> tangents;
  tangents->SetInputConnection(triangulation->GetOutputPort());

  svtkNew<svtkPNGReader> png;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/normalMapping.png");
  png->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(png->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetNormalTexture(texture);
  renderer->AddActor(actor);

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
