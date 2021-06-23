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

#include "svtkmWarpVector.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetMapper.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

int TestSVTKMWarpVector(int argc, char* argv[])
{
  svtkNew<svtkRenderer> xyplaneRen, dataNormalRen;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Define viewport ranges
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = { 0.0, 0.0, 0.5, 1.0 };
  double centerViewport[4] = { 0.5, 0.0, 1.0, 1.0 };

  /// First window - xy plane
  svtkSmartPointer<svtkRTAnalyticSource> xySource = svtkSmartPointer<svtkRTAnalyticSource>::New();
  xySource->SetWholeExtent(-100, 100, -100, 100, 1, 1);
  xySource->SetCenter(0, 0, 0);
  xySource->SetMaximum(255);
  xySource->SetStandardDeviation(.5);
  xySource->SetXFreq(60);
  xySource->SetYFreq(30);
  xySource->SetZFreq(40);
  xySource->SetXMag(10);
  xySource->SetYMag(18);
  xySource->SetZMag(10);
  xySource->SetSubsampleRate(1);
  xySource->Update();
  svtkNew<svtkFloatArray> xyVector;
  xyVector->SetNumberOfComponents(3);
  xyVector->SetName("scalarVector");
  xyVector->SetNumberOfTuples(xySource->GetOutput()->GetNumberOfPoints());
  for (svtkIdType i = 0; i < xySource->GetOutput()->GetNumberOfPoints(); i++)
  {
    xyVector->SetTuple3(i, 0.0, 0.0, 1.0);
  }
  xySource->GetOutput()->GetPointData()->AddArray(xyVector);

  svtkNew<svtkmWarpVector> xyWarpVector;
  xyWarpVector->SetScaleFactor(2);
  xyWarpVector->SetInputConnection(xySource->GetOutputPort());

  // Create a scalarVector array
  xyWarpVector->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "scalarVector");
  xyWarpVector->Update();

  svtkNew<svtkDataSetMapper> xyplaneMapper;
  xyplaneMapper->SetInputConnection(xyWarpVector->GetOutputPort());

  svtkNew<svtkActor> xyplaneActor;
  xyplaneActor->SetMapper(xyplaneMapper);

  renWin->AddRenderer(xyplaneRen);
  xyplaneRen->SetViewport(leftViewport);
  xyplaneRen->SetBackground(0.5, 0.4, 0.3);
  xyplaneRen->AddActor(xyplaneActor);

  /// Second window - data normal
  svtkSmartPointer<svtkSphereSource> dataNormalSource = svtkSmartPointer<svtkSphereSource>::New();
  dataNormalSource->SetRadius(100);
  dataNormalSource->SetThetaResolution(20);
  dataNormalSource->SetPhiResolution(20);
  dataNormalSource->Update();
  auto dataNormalSourceOutput = dataNormalSource->GetOutput();

  svtkNew<svtkmWarpVector> dataNormalWarpVector;
  dataNormalWarpVector->SetScaleFactor(5);
  dataNormalWarpVector->SetInputData(dataNormalSource->GetOutput());
  dataNormalWarpVector->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::POINT, dataNormalSourceOutput->GetPointData()->GetNormals()->GetName());

  svtkNew<svtkDataSetMapper> dataNormalMapper;
  dataNormalMapper->SetInputConnection(dataNormalWarpVector->GetOutputPort());

  svtkNew<svtkActor> dataNormalActor;
  dataNormalActor->SetMapper(dataNormalMapper);

  renWin->AddRenderer(dataNormalRen);
  dataNormalRen->SetViewport(centerViewport);
  dataNormalRen->SetBackground(0.0, 0.7, 0.2);
  dataNormalRen->AddActor(dataNormalActor);

  xyplaneRen->ResetCamera();
  dataNormalRen->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
