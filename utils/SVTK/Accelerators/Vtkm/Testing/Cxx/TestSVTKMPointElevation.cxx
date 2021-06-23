//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTriangleFilter.h"
#include "svtkmPointElevation.h"

namespace
{
int RunSVTKPipeline(svtkPlaneSource* plane, int argc, char* argv[])
{
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  svtkNew<svtkTriangleFilter> tf;
  tf->SetInputConnection(plane->GetOutputPort());
  tf->Update();

  svtkNew<svtkPolyData> pd;
  pd->CopyStructure(tf->GetOutput());
  svtkIdType numPts = pd->GetNumberOfPoints();
  svtkPoints* oldPts = tf->GetOutput()->GetPoints();
  svtkNew<svtkPoints> newPts;
  newPts->SetNumberOfPoints(numPts);
  for (svtkIdType i = 0; i < numPts; i++)
  {
    auto pt = oldPts->GetPoint(i);
    auto r = sqrt(pow(pt[0], 2) + pow(pt[1], 2));
    auto z = 1.5 * cos(2 * r);
    newPts->SetPoint(i, pt[0], pt[1], z);
  }
  pd->SetPoints(newPts);

  svtkNew<svtkmPointElevation> pe;
  pe->SetInputData(pd);
  pe->SetLowPoint(0, 0, -1.5);
  pe->SetHighPoint(0, 0, 1.5);
  pe->SetScalarRange(-1.5, 1.5);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pe->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("elevation");

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // Add the actor to the renderer, set the background and size
  ren->AddActor(actor);

  ren->SetBackground(0, 0, 0);
  svtkNew<svtkCamera> camera;
  camera->SetPosition(1, 50, 50);
  ren->SetActiveCamera(camera);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
} //  Anonymous namespace

int TestSVTKMPointElevation(int argc, char* argv[])
{
  // Create a plane source
  svtkNew<svtkPlaneSource> plane;
  int res = 200;
  plane->SetXResolution(res);
  plane->SetYResolution(res);
  plane->SetOrigin(-10, -10, 0);
  plane->SetPoint1(10, -10, 0);
  plane->SetPoint2(-10, 10, 0);

  // Run the pipeline
  return RunSVTKPipeline(plane, argc, argv);
}
