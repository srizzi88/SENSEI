/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test ensure that when all translucent fragments are in front of opaque fragments, the
// occlusion query check does not exit too early
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCubeSource.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

int TestDepthPeelingOcclusionQuery(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkPolyDataMapper> mapperBox;
  svtkNew<svtkCubeSource> box;
  box->SetXLength(3.0);
  box->SetYLength(3.0);
  mapperBox->SetInputConnection(box->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapperSphere;
  svtkNew<svtkSphereSource> sphere;
  mapperSphere->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> actorBox;
  actorBox->GetProperty()->SetColor(0.1, 0.1, 0.1);
  actorBox->SetMapper(mapperBox);
  renderer->AddActor(actorBox);

  svtkNew<svtkActor> actorSphere1;
  actorSphere1->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actorSphere1->GetProperty()->SetOpacity(0.2);
  actorSphere1->SetPosition(0.0, 0.0, 1.0);
  actorSphere1->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere1);

  svtkNew<svtkActor> actorSphere2;
  actorSphere2->GetProperty()->SetColor(0.0, 1.0, 0.0);
  actorSphere2->GetProperty()->SetOpacity(0.2);
  actorSphere2->SetPosition(0.0, 0.0, 2.0);
  actorSphere2->SetMapper(mapperSphere);
  renderer->AddActor(actorSphere2);

  renderer->SetUseDepthPeeling(1);
  renderer->SetOcclusionRatio(0.0);
  renderer->SetMaximumNumberOfPeels(20);

  renWin->SetSize(500, 500);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
