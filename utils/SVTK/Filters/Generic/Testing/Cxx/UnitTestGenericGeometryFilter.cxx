/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestGenericGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkBridgeDataSet.h"
#include "svtkCellData.h"
#include "svtkGenericGeometryFilter.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPolyData.h"
#include "svtkTetra.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVertex.h"

#include "svtkTestErrorObserver.h"

#include <sstream>

static svtkSmartPointer<svtkBridgeDataSet> CreatePolyData(const int xres, const int yres);
static svtkSmartPointer<svtkBridgeDataSet> CreateVertexData();
static svtkSmartPointer<svtkBridgeDataSet> CreateTetraData();

int UnitTestGenericGeometryFilter(int, char*[])
{
  const int xres = 20, yres = 10;
  int status = EXIT_SUCCESS;
  {
    std::cout << "Testing empty print...";
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    std::ostringstream emptyPrint;
    filter->Print(emptyPrint);
    std::cout << "PASSED." << std::endl;
  }
  {
    std::cout << "Testing default settings...";
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->SetInputData(CreatePolyData(xres, yres));
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << "# of cells: " << got;
    int expected = xres * yres;
    if (expected != got)
    {
      std::cout << " Expected " << expected << " cells"
                << " but got " << got << " cells."
                << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing PointClippingOn()...";
    svtkSmartPointer<svtkPointLocator> locator = svtkSmartPointer<svtkPointLocator>::New();
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->SetInputData(CreatePolyData(xres, yres));
    filter->SetLocator(locator);
    filter->MergingOff();
    filter->PointClippingOn();
    filter->CellClippingOff();
    filter->ExtentClippingOff();
    filter->SetPointMinimum(0);
    filter->SetPointMaximum((xres + 1) * (yres + 1) - 1);
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << "# of cells: " << got;
    int expected = xres * yres;
    if (expected != got)
    {
      std::cout << " Expected " << expected << " cells"
                << " but got " << got << " cells."
                << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
    std::ostringstream fullPrint;
    filter->Print(fullPrint);
  }
  {
    std::cout << "Testing CellClippingOn()...";
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->SetInputData(CreatePolyData(xres, yres));
    filter->PointClippingOff();
    filter->CellClippingOn();
    filter->ExtentClippingOff();
    filter->SetCellMinimum(xres);
    filter->SetCellMaximum(xres + 9);
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << "# of cells: " << got;
    int expected = filter->GetCellMaximum() - filter->GetCellMinimum() + 1;
    if (expected != got)
    {
      std::cout << " Expected " << expected << " cells"
                << " but got " << got << " cells."
                << " FAILED" << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing ExtentClippingOn()...";
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->MergingOn();
    filter->SetInputData(CreatePolyData(xres, yres));
    filter->PointClippingOff();
    filter->CellClippingOff();
    filter->ExtentClippingOn();
    filter->PassThroughCellIdsOn();
    filter->SetExtent(.4, -.4, .4, -.4, .4, -.4);
    filter->SetExtent(-.499, .499, -.499, .499, 0.0, 0.0);
    filter->SetExtent(-.499, .499, -.499, .499, 0.0, 0.0);
    filter->Update();
    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << "# of cells: " << got;
    int expected = (xres * yres) - 2 * xres - 2 * (yres - 2);
    if (expected != got)
    {
      std::cout << " Expected " << expected << " cells"
                << " but got " << got << " cells."
                << " FAILED." << std::endl;
      status++;
    }
    else if (filter->GetOutput()->GetCellData()->GetArray("svtkOriginalCellIds") == nullptr)
    {
      std::cout << " PassThroughCellIdsOn should produce svtkOriginalCellIds, but did not."
                << std::endl;
      std::cout << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing with TetraData...";
    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->SetInputData(CreateTetraData());
    filter->PointClippingOff();
    filter->CellClippingOff();
    filter->ExtentClippingOff();
    filter->PassThroughCellIdsOn();
    filter->Update();

    int got = filter->GetOutput()->GetNumberOfCells();
    std::cout << "# of cells: " << got;
    int expected = 4;
    if (expected != got)
    {
      std::cout << " Expected " << expected << " cells"
                << " but got " << got << " cells."
                << " FAILED." << std::endl;
      status++;
    }
    else
    {
      std::cout << " PASSED." << std::endl;
    }
  }
  {
    std::cout << "Testing errors...";
    svtkSmartPointer<svtkTest::ErrorObserver> errorObserver =
      svtkSmartPointer<svtkTest::ErrorObserver>::New();

    svtkSmartPointer<svtkGenericGeometryFilter> filter =
      svtkSmartPointer<svtkGenericGeometryFilter>::New();
    filter->AddObserver(svtkCommand::ErrorEvent, errorObserver);
    filter->SetInputData(svtkSmartPointer<svtkBridgeDataSet>::New());
    filter->Update();
    status += errorObserver->CheckErrorMessage("Number of cells is zero, no data to process.");

    filter->SetInputData(CreateVertexData());
    filter->Update();
    status += errorObserver->CheckErrorMessage("Cell of dimension 0 not handled yet.");

    if (status)
    {
      std::cout << "FAILED." << std::endl;
    }
    else
    {
      std::cout << "PASSED." << std::endl;
    }
  }
  return status;
}

svtkSmartPointer<svtkBridgeDataSet> CreatePolyData(const int xres, const int yres)
{
  svtkSmartPointer<svtkPlaneSource> plane = svtkSmartPointer<svtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();
  svtkSmartPointer<svtkIntArray> cellData = svtkSmartPointer<svtkIntArray>::New();
  cellData->SetNumberOfTuples(xres * yres);
  cellData->SetName("CellDataTestArray");
  svtkIdType c = 0;
  for (int j = 0; j < yres; ++j)
  {
    for (int i = 0; i < xres; ++i)
    {
      cellData->SetTuple1(c++, i);
    }
  }
  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples((xres + 1) * (yres + 1));
  pointData->SetName("PointDataTestArray");
  c = 0;
  for (int j = 0; j < yres + 1; ++j)
  {
    for (int i = 0; i < xres + 1; ++i)
    {
      pointData->SetTuple1(c++, i);
    }
  }

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();
  pd = plane->GetOutput();
  pd->GetPointData()->SetScalars(pointData);
  pd->GetCellData()->SetScalars(cellData);

  svtkSmartPointer<svtkBridgeDataSet> bridge = svtkSmartPointer<svtkBridgeDataSet>::New();
  bridge->SetDataSet(plane->GetOutput());

  return bridge;
}
svtkSmartPointer<svtkBridgeDataSet> CreateVertexData()
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);

  svtkSmartPointer<svtkVertex> vertex = svtkSmartPointer<svtkVertex>::New();
  vertex->GetPointIds()->SetId(0, 0);

  svtkSmartPointer<svtkCellArray> vertices = svtkSmartPointer<svtkCellArray>::New();
  vertices->InsertNextCell(vertex);

  svtkSmartPointer<svtkPolyData> polydata = svtkSmartPointer<svtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetVerts(vertices);

  svtkSmartPointer<svtkBridgeDataSet> bridge = svtkSmartPointer<svtkBridgeDataSet>::New();
  bridge->SetDataSet(polydata);

  return bridge;
}

svtkSmartPointer<svtkBridgeDataSet> CreateTetraData()
{
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  svtkSmartPointer<svtkTetra> tetra = svtkSmartPointer<svtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 4);
  tetra->GetPointIds()->SetId(1, 5);
  tetra->GetPointIds()->SetId(2, 6);
  tetra->GetPointIds()->SetId(3, 7);

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(SVTK_TETRA, cellArray);

  svtkSmartPointer<svtkIntArray> pointData = svtkSmartPointer<svtkIntArray>::New();
  pointData->SetNumberOfTuples(unstructuredGrid->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  int c = 0;
  for (svtkIdType id = 0; id < tetra->GetNumberOfPoints(); ++id)
  {
    pointData->SetTuple1(c++, id);
  }
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  svtkSmartPointer<svtkBridgeDataSet> bridge = svtkSmartPointer<svtkBridgeDataSet>::New();
  bridge->SetDataSet(unstructuredGrid);

  return bridge;
}
