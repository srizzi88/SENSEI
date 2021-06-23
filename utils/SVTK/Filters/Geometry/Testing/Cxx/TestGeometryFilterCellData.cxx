/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeometryFilterCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test that the proper amount of tuples exist in the
// point and cell data arrays after the svtkGeometryFilter
// is used.

#include <iostream>

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkGeometryFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

int TestGeometryFilter(svtkUnstructuredGrid* ug);
int CheckDataSet(svtkDataSet* d);
int CheckFieldData(svtkIdType numGridEntities, svtkFieldData* fd);

// Creates a svtkUnstructuredGrid
class GridFactory
{
public:
  GridFactory();

  void AddVertexCells();
  void AddLineCells();
  void AddTriangleCells();
  void AddTetraCells();
  svtkUnstructuredGrid* Get();

private:
  svtkSmartPointer<svtkUnstructuredGrid> Grid;
};

GridFactory::GridFactory()
  : Grid(svtkSmartPointer<svtkUnstructuredGrid>::New())
{
  // the points
  static float x[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };

  std::cout << "Defining 8 points\n";
  // Create the points
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->SetNumberOfPoints(8);
  for (int i = 0; i < 8; ++i)
  {
    points->SetPoint(i, x[i]);
  }
  this->Grid->SetPoints(points);
}

// Create 2 tetras
void GridFactory::AddTetraCells()
{
  if (!this->Grid)
    return;

  std::cout << "Adding 2 tetra cells\n";
  svtkIdType pts[4];
  pts[0] = 0;
  pts[1] = 1;
  pts[2] = 2;
  pts[3] = 3;
  this->Grid->InsertNextCell(SVTK_TETRA, 4, pts);
  pts[0] = 2;
  pts[1] = 3;
  pts[2] = 4;
  pts[3] = 5;
  this->Grid->InsertNextCell(SVTK_TETRA, 4, pts);
}

// Create 2 triangles
void GridFactory::AddTriangleCells()
{
  if (!this->Grid)
    return;

  std::cout << "Adding 2 triangle cells\n";

  svtkIdType pts[3];
  pts[0] = 1;
  pts[1] = 3;
  pts[2] = 5;
  this->Grid->InsertNextCell(SVTK_TRIANGLE, 3, pts);
  pts[0] = 2;
  pts[1] = 4;
  pts[2] = 6;
  this->Grid->InsertNextCell(SVTK_TRIANGLE, 3, pts);
}

// Create 2 lines
void GridFactory::AddLineCells()
{
  if (!this->Grid)
    return;

  std::cout << "Adding 2 line cells\n";

  svtkIdType pts[2];
  pts[0] = 3;
  pts[1] = 7;
  this->Grid->InsertNextCell(SVTK_LINE, 2, pts);
  pts[0] = 0;
  pts[1] = 4;
  this->Grid->InsertNextCell(SVTK_LINE, 2, pts);
}

// Create 2 points
void GridFactory::AddVertexCells()
{
  if (!this->Grid)
    return;

  std::cout << "Adding 2 vertex cells\n";

  svtkIdType pts[1];
  pts[0] = 7;
  this->Grid->InsertNextCell(SVTK_VERTEX, 1, pts);
  pts[0] = 6;
  this->Grid->InsertNextCell(SVTK_VERTEX, 1, pts);
}

// Add cell data and point data for all cells/points, and
// return the unstructured grid.
svtkUnstructuredGrid* GridFactory::Get()
{
  if (!this->Grid)
    return nullptr;

  // Create a point data array
  const char* name = "foo";
  int num = this->Grid->GetNumberOfPoints();
  std::cout << "Adding point data array '" << name << "' with data for " << num << " points\n";
  svtkSmartPointer<svtkIdTypeArray> pointDataArray = svtkSmartPointer<svtkIdTypeArray>::New();
  pointDataArray->SetName(name);
  // Creating data for 8 points
  for (int i = 0; i < num; ++i)
  {
    svtkIdType value = i + 100;
    pointDataArray->InsertNextTypedTuple(&value);
  }
  this->Grid->GetPointData()->AddArray(pointDataArray);

  // Create the cell data array
  name = "bar";
  num = this->Grid->GetNumberOfCells();
  std::cout << "Adding cell data array '" << name << "' with data for " << num << " cells\n";
  svtkSmartPointer<svtkIdTypeArray> cellDataArray = svtkSmartPointer<svtkIdTypeArray>::New();
  cellDataArray->SetName(name);
  cellDataArray->SetNumberOfComponents(1);
  for (int i = 0; i < num; ++i)
  {
    svtkIdType value = i + 200;
    cellDataArray->InsertNextTypedTuple(&value);
  }
  this->Grid->GetCellData()->AddArray(cellDataArray);

  return this->Grid;
}

int TestGeometryFilterCellData(int, char*[])
{
  svtkSmartPointer<svtkUnstructuredGrid> ug;

  // Build the unstructured grid
  GridFactory g;
  g.AddTetraCells();
  g.AddTriangleCells();
  g.AddLineCells();
  g.AddVertexCells();
  ug = g.Get();

  // Run it through svtkGeometryFilter
  int retVal = TestGeometryFilter(ug);

  return retVal;
}

// Runs the unstructured grid through the svtkGeometryFilter and prints the
// output
int TestGeometryFilter(svtkUnstructuredGrid* ug)
{
  // Print the input unstructured grid dataset
  std::cout << "\nsvtkGeometryFilter input:\n";
  int retVal = CheckDataSet(ug);

  // Do the filtering
  svtkSmartPointer<svtkGeometryFilter> gf = svtkSmartPointer<svtkGeometryFilter>::New();
  gf->SetInputData(ug);
  gf->Update();

  // Print the output poly data
  std::cout << "\nsvtkGeometryFilter output:\n";
  svtkPolyData* poly = svtkPolyData::SafeDownCast(gf->GetOutput());
  retVal += CheckDataSet(poly);
  return retVal;
}

int CheckDataSet(svtkDataSet* d)
{
  if (!d)
  {
    std::cout << "No dataset\n";
    return 1;
  }

  const char* name;
  if (svtkUnstructuredGrid::SafeDownCast(d))
    name = "svtkUnstructuredGrid";
  else if (svtkPolyData::SafeDownCast(d))
    name = "svtkPolyData";
  else
    name = "svtkDataSet";

  std::cout << name << " dimensions: #cells=" << d->GetNumberOfCells()
            << " #points=" << d->GetNumberOfPoints() << "\n";
  int retVal = CheckFieldData(d->GetNumberOfPoints(), d->GetPointData());
  retVal += CheckFieldData(d->GetNumberOfCells(), d->GetCellData());
  return retVal;
}

int CheckFieldData(svtkIdType numGridEntities, svtkFieldData* fd)
{
  int retVal = 0;
  if (!fd)
  {
    std::cout << "No field data\n";
    return 1;
  }

  const char* name;
  if (svtkCellData::SafeDownCast(fd))
  {
    name = "cell data";
  }
  else if (svtkPointData::SafeDownCast(fd))
  {
    name = "point data";
  }
  else
  {
    name = "field data";
  }

  for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
  {
    svtkAbstractArray* a = fd->GetArray(i);
    if (a->GetNumberOfTuples() != numGridEntities)
    {
      svtkGenericWarningMacro(<< name << " array '" << a->GetName() << "' has #tuples="
                             << a->GetNumberOfTuples() << " but should have " << numGridEntities);
      retVal = 1;
    }
  }
  return retVal;
}
