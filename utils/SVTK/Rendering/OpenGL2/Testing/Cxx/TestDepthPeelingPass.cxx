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
#include "svtkCellArray.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTestUtilities.h"

int TestDepthPeelingPass(int argc, char* argv[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkPolyDataMapper> mapper;
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());

  // create three dragons
  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(1.0, 0.0, 0.0);
    actor->GetProperty()->SetDiffuseColor(1.0, 0.8, 0.3);
    actor->GetProperty()->SetSpecular(0.0);
    actor->GetProperty()->SetDiffuse(0.5);
    actor->GetProperty()->SetAmbient(0.3);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(-0.1, 0.0, -0.1);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
    actor->GetProperty()->SetDiffuseColor(0.2, 1.0, 0.8);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.2);
    actor->GetProperty()->SetDiffuse(0.9);
    actor->GetProperty()->SetAmbient(0.1);
    actor->GetProperty()->SetSpecularPower(10.0);
    actor->GetProperty()->SetOpacity(0.20);
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.5, 0.65, 1.0);
    actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetSpecular(0.7);
    actor->GetProperty()->SetDiffuse(0.4);
    actor->GetProperty()->SetSpecularPower(60.0);
    actor->GetProperty()->SetOpacity(0.35);
    actor->SetPosition(0.1, 0.0, 0.1);
    renderer->AddActor(actor);
  }

  renderer->SetUseDepthPeeling(1);
  renderer->SetOcclusionRatio(0.0);
  renderer->SetMaximumNumberOfPeels(20);

  renWin->SetSize(500, 500);
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->SetBackground2(.3, .1, .2);
  renderer->GradientBackgroundOn();
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(15.0);
  renderer->GetActiveCamera()->Zoom(1.8);

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
