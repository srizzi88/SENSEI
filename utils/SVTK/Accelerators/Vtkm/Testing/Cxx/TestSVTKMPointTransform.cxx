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
#include "svtkTransform.h"
#include "svtkTriangleFilter.h"
#include "svtkmPointTransform.h"

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

  svtkNew<svtkmPointTransform> pf;
  pf->SetInputData(pd);
  svtkNew<svtkTransform> transformMatrix;
  transformMatrix->RotateX(30);
  transformMatrix->RotateY(60);
  transformMatrix->RotateZ(90);
  pf->SetTransform(transformMatrix);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pf->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);

  ren->SetBackground(0.0, 0.0, 0.0);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
}

int TestSVTKMPointTransform(int argc, char* argv[])
{
  svtkNew<svtkPlaneSource> plane;
  int res = 300;
  plane->SetXResolution(res);
  plane->SetYResolution(res);
  plane->SetOrigin(-10.0, -10.0, 0.0);
  plane->SetPoint1(10.0, -10.0, 0.0);
  plane->SetPoint2(-10.0, 10.0, 0.0);

  return RunSVTKPipeline(plane, argc, argv);
}
