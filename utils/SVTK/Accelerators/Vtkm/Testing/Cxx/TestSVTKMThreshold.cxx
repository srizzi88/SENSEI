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
#include "svtkDataSetSurfaceFilter.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkThreshold.h"
#include "svtkTrivialProducer.h"
#include "svtkmThreshold.h"

namespace
{
void fillElevationArray(svtkFloatArray* elven, svtkImageData* grid)
{
  elven->SetName("Elevation");
  const svtkIdType size = grid->GetNumberOfPoints();
  elven->SetNumberOfValues(size);
  double pos[3] = { 0, 0, 0 };
  for (svtkIdType i = 0; i < size; ++i)
  {
    grid->GetPoint(i, pos);
    elven->SetValue(i, sqrt(svtkMath::Dot(pos, pos)));
  }
}

int RunSVTKPipeline(svtkImageData* grid, int argc, char* argv[])
{
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  // compute an elevation array
  svtkNew<svtkFloatArray> elevationPoints;
  fillElevationArray(elevationPoints, grid);
  grid->GetPointData()->AddArray(elevationPoints);

  svtkNew<svtkTrivialProducer> producer;
  producer->SetOutput(grid);

  svtkNew<svtkmThreshold> threshold;
  threshold->SetInputConnection(producer->GetOutputPort());
  threshold->SetPointsDataTypeToFloat();
  threshold->AllScalarsOn();
  threshold->ThresholdBetween(0, 100);
  threshold->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");

  svtkNew<svtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(threshold->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Elevation");
  mapper->SetScalarRange(0.0, 100.0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbient(1.0);
  actor->GetProperty()->SetDiffuse(0.0);

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

} // Anonymous namespace

int TestSVTKMThreshold(int argc, char* argv[])
{
  // create the sample grid
  svtkNew<svtkImageData> grid;
  int dim = 128;
  grid->SetOrigin(0.0, 0.0, 0.0);
  grid->SetSpacing(1.0, 1.0, 1.0);
  grid->SetExtent(0, dim - 1, 0, dim - 1, 0, dim - 1);

  // run the pipeline
  return RunSVTKPipeline(grid, argc, argv);
}
