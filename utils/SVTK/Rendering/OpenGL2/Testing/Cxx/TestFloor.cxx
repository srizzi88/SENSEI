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
#include "svtkImageGridSource.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShaderProgram.h"
#include "svtkSkybox.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"

#include "svtkLight.h"

//----------------------------------------------------------------------------
int TestFloor(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  svtkNew<svtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkImageGridSource> grid;
  grid->SetGridSpacing(32, 32, 0);

  svtkNew<svtkTexture> texture;
  texture->SetColorModeToMapScalars();
  svtkNew<svtkLookupTable> lut;
  texture->SetLookupTable(lut);
  lut->SetSaturationRange(0.0, 0.0);
  lut->SetValueRange(0.0, 1.0);
  lut->SetTableRange(0.0, 1.0);
  lut->Build();
  texture->InterpolateOn();
  texture->RepeatOn();
  texture->MipmapOn();
  texture->SetInputConnection(grid->GetOutputPort(0));

  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetPosition(0, 0, 0);
  actor->SetScale(6.0, 6.0, 6.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  svtkNew<svtkSkybox> floor;
  floor->SetProjectionToFloor();
  floor->SetTexture(texture);
  renderer->AddActor(floor);

  renderer->GetActiveCamera()->SetPosition(0.0, 0.55, 2.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.55, 0.0);
  renderer->GetActiveCamera()->SetViewAngle(60.0);
  renderer->GetActiveCamera()->Zoom(1.1);
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(5);
  renderer->GetActiveCamera()->Roll(-10);
  renderer->SetBackground(0.6, 0.7, 1.0);
  renderer->ResetCameraClippingRange();

  renderWindow->Render();

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
