/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkElevationFilter.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkmThreshold.h"

int TestSVTKMThreshold2(int argc, char* argv[])
{
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  //---------------------------------------------------
  // Test using different thresholding methods
  //---------------------------------------------------
  svtkNew<svtkRTAnalyticSource> source;

  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(source->GetOutputPort());
  elevation->SetScalarRange(0.0, 1.0);
  elevation->SetLowPoint(-10.0, -10.0, -10.0);
  elevation->SetHighPoint(10.0, 10.0, 10.0);

  svtkNew<svtkmThreshold> threshold;
  threshold->SetInputConnection(elevation->GetOutputPort());
  threshold->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");

  double L = 100;
  double U = 200;
  threshold->ThresholdBetween(L, U);
  threshold->SetAllScalars(0);
  threshold->Update();

  threshold->UseContinuousCellRangeOn();
  threshold->Update();

  svtkNew<svtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(threshold->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Elevation");
  mapper->SetScalarRange(0.0, 1.0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
