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
#include "svtkmExternalFaces.h"

#include "svtkActor.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCylinder.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRandomAttributeGenerator.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"
#include "svtkUnstructuredGrid.h"

#include "svtkDataArray.h"
#include "svtkPointData.h"

namespace
{

bool Convert2DUnstructuredGridToPolyData(svtkUnstructuredGrid* in, svtkPolyData* out)
{
  svtkIdType numCells = in->GetNumberOfCells();
  out->AllocateEstimate(numCells, 1);
  out->SetPoints(in->GetPoints());

  for (svtkIdType i = 0; i < numCells; ++i)
  {
    svtkCell* cell = in->GetCell(i);
    if (cell->GetCellType() != SVTK_TRIANGLE && cell->GetCellType() != SVTK_QUAD)
    {
      std::cout << "Error: Unexpected cell type: " << cell->GetCellType() << "\n";
      return false;
    }
    out->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
  }

  out->GetPointData()->PassData(in->GetPointData());
  return true;
}

} // anonymous namespace

int TestSVTKMExternalFaces(int argc, char* argv[])
{
  // create pipeline
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(15);
  cylinder->SetAxis(0, 1, 0);
  svtkNew<svtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetInputConnection(wavelet->GetOutputPort());
  clipCyl->SetClipFunction(cylinder);
  clipCyl->InsideOutOn();

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(12);
  svtkNew<svtkTableBasedClipDataSet> clipSphr;
  clipSphr->SetInputConnection(clipCyl->GetOutputPort());
  clipSphr->SetClipFunction(sphere);

  svtkNew<svtkTransform> transform;
  transform->RotateZ(45);
  svtkNew<svtkTransformFilter> transFilter;
  transFilter->SetInputConnection(clipSphr->GetOutputPort());
  transFilter->SetTransform(transform);

  svtkNew<svtkRandomAttributeGenerator> cellDataAdder;
  cellDataAdder->SetInputConnection(transFilter->GetOutputPort());
  cellDataAdder->SetDataTypeToFloat();
  cellDataAdder->GenerateCellVectorsOn();

  svtkNew<svtkmExternalFaces> externalFaces;
  externalFaces->SetInputConnection(cellDataAdder->GetOutputPort());

  // execute pipeline
  externalFaces->Update();
  svtkUnstructuredGrid* result = externalFaces->GetOutput();

  svtkIdType numInputPoints = result->GetNumberOfPoints();

  externalFaces->CompactPointsOn();
  externalFaces->Update();
  result = externalFaces->GetOutput();

  if (result->GetNumberOfPoints() >= numInputPoints)
  {
    std::cout << "Expecting the number of points in the output to be less "
              << "than the input (" << result->GetNumberOfPoints() << ">=" << numInputPoints
              << ")\n";
    return 1;
  }

  if (result->GetCellData()->GetArray("RandomCellVectors")->GetNumberOfTuples() !=
    result->GetNumberOfCells())
  {
    std::cout << "Expecting a cell field with number of entries equal to "
              << "the number of cells";
    return 1;
  }

  svtkNew<svtkPolyData> polydata;
  if (!Convert2DUnstructuredGridToPolyData(result, polydata))
  {
    std::cout << "Error converting result to polydata\n";
    return 1;
  }

  // render results
  double scalarRange[2];
  polydata->GetPointData()->GetArray("RTData")->GetRange(scalarRange);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(scalarRange);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
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
