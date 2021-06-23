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

#include "svtkmWarpScalar.h"

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

int TestSVTKMWarpScalar(int argc, char* argv[])
{
  svtkNew<svtkRenderer> xyplaneRen, dataNormalRen, customNormalRen;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(900, 300);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Define viewport ranges
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = { 0.0, 0.0, 0.33, 1.0 };
  double centerViewport[4] = { 0.33, 0.0, .66, 1.0 };
  double rightViewport[4] = { 0.66, 0.0, 1.0, 1.0 };

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
  xySource->SetZMag(5);
  xySource->SetSubsampleRate(1);

  svtkNew<svtkmWarpScalar> xyWarpScalar;
  xyWarpScalar->SetScaleFactor(2);
  xyWarpScalar->XYPlaneOn();
  xyWarpScalar->SetNormal(1, 0, 0); // should be ignored
  xyWarpScalar->SetInputConnection(xySource->GetOutputPort());
  xyWarpScalar->Update();
  svtkPointSet* points = xyWarpScalar->GetOutput();
  for (svtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    assert(points->GetPoint(i)[2] == 3.0);
    if (points->GetPoint(i)[2] != 3.0)
    {
      std::cout << "XYPlane result is wrong at i=" << i << std::endl;
    }
  }

  svtkNew<svtkDataSetMapper> xyplaneMapper;
  xyplaneMapper->SetInputConnection(xyWarpScalar->GetOutputPort());

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
  // Create a scalar array
  auto dataNormalSourceOutput = dataNormalSource->GetOutput();
  svtkNew<svtkFloatArray> scalarArray;
  scalarArray->SetName("scalarfactor");
  scalarArray->SetNumberOfValues(dataNormalSourceOutput->GetNumberOfPoints());
  for (svtkIdType i = 0; i < dataNormalSourceOutput->GetNumberOfPoints(); i++)
  {
    scalarArray->SetValue(i, 2);
  }
  dataNormalSourceOutput->GetPointData()->AddArray(scalarArray);

  svtkNew<svtkmWarpScalar> dataNormalWarpScalar;
  dataNormalWarpScalar->SetScaleFactor(2);
  dataNormalWarpScalar->SetInputData(dataNormalSource->GetOutput());
  dataNormalWarpScalar->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "scalarfactor");
  svtkNew<svtkDataSetMapper> dataNormalMapper;
  dataNormalMapper->SetInputConnection(dataNormalWarpScalar->GetOutputPort());

  svtkNew<svtkActor> dataNormalActor;
  dataNormalActor->SetMapper(dataNormalMapper);

  renWin->AddRenderer(dataNormalRen);
  dataNormalRen->SetViewport(centerViewport);
  dataNormalRen->SetBackground(0.0, 0.7, 0.2);
  dataNormalRen->AddActor(dataNormalActor);

  /// Third window - custom normal
  svtkSmartPointer<svtkRTAnalyticSource> customNormalSource =
    svtkSmartPointer<svtkRTAnalyticSource>::New();
  customNormalSource->SetWholeExtent(-100, 100, -100, 100, 1, 1);
  customNormalSource->SetCenter(0, 0, 0);
  customNormalSource->SetMaximum(255);
  customNormalSource->SetStandardDeviation(.5);
  customNormalSource->SetXFreq(60);
  customNormalSource->SetYFreq(30);
  customNormalSource->SetZFreq(40);
  customNormalSource->SetXMag(10);
  customNormalSource->SetYMag(18);
  customNormalSource->SetZMag(5);
  customNormalSource->SetSubsampleRate(1);

  svtkNew<svtkmWarpScalar> customNormalWarpScalar;
  customNormalWarpScalar->SetScaleFactor(2);
  customNormalWarpScalar->SetNormal(0.333, 0.333, 0.333);
  customNormalWarpScalar->SetInputConnection(customNormalSource->GetOutputPort());
  svtkNew<svtkDataSetMapper> customNormalMapper;
  customNormalMapper->SetInputConnection(customNormalWarpScalar->GetOutputPort());

  svtkNew<svtkActor> customNormalActor;
  customNormalActor->SetMapper(customNormalMapper);
  renWin->AddRenderer(customNormalRen);
  customNormalRen->SetViewport(rightViewport);
  customNormalRen->SetBackground(0.3, 0.2, 0.5);
  customNormalRen->AddActor(customNormalActor);

  xyplaneRen->ResetCamera();
  dataNormalRen->ResetCamera();
  customNormalRen->ResetCamera();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return (!retVal);
}
