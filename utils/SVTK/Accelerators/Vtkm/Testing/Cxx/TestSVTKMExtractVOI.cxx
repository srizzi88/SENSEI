/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmExtractVOI.h"

#include "svtkActor.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTriangleFilter.h"

#include "svtkImageData.h"

int TestSVTKMExtractVOI(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(2.0);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  svtkNew<svtkRTAnalyticSource> rt;
  rt->SetWholeExtent(-50, 50, -50, 50, 0, 0);

  svtkNew<svtkmExtractVOI> voi;
  voi->SetInputConnection(rt->GetOutputPort());
  voi->SetVOI(-11, 39, 5, 45, 0, 0);
  voi->SetSampleRate(5, 5, 1);

  // Get rid of ambiguous triagulation issues.
  svtkNew<svtkDataSetSurfaceFilter> surf;
  surf->SetInputConnection(voi->GetOutputPort());

  svtkNew<svtkTriangleFilter> tris;
  tris->SetInputConnection(surf->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tris->GetOutputPort());
  mapper->SetScalarRange(130, 280);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(sphereActor);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
