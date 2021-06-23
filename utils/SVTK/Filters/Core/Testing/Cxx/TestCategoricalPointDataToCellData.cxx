/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalPointDataToCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCellArray.h>
#include <svtkCellData.h>
#include <svtkDataSetSurfaceFilter.h>
#include <svtkDoubleArray.h>
#include <svtkPointData.h>
#include <svtkPointDataToCellData.h>
#include <svtkPointLocator.h>
#include <svtkPoints.h>
#include <svtkPolyDataMapper.h>
#include <svtkSmartPointer.h>
#include <svtkTriangle.h>
#include <svtkUnstructuredGrid.h>

namespace
{
static const double EPSILON = 1.e-6;

// Create a triangle with vertices p0, p1, p2, add the points to the point
// locator, and add the triangle to the cell array.
void AddTriangle(const double* p0, const double* p1, const double* p2,
  svtkPointLocator* pointLocator, svtkCellArray* cells)
{
  svtkSmartPointer<svtkTriangle> t = svtkSmartPointer<svtkTriangle>::New();

  svtkIdType bIndices[3][3] = { { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 } };

  double p[3];
  svtkIdType pId, *bIndex;
  for (svtkIdType i = 0; i < 3; i++)
  {
    bIndex = bIndices[i];
    for (svtkIdType j = 0; j < 3; j++)
    {
      p[j] = (p0[j] * bIndex[2]) + (p1[j] * bIndex[0]) + (p2[j] * bIndex[1]);
    }
    pointLocator->InsertUniquePoint(p, pId);
    t->GetPointIds()->SetId(i, pId);
  }
  cells->InsertNextCell(t);
}
}

int TestCategoricalPointDataToCellData(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  // Construct an unstructured grid of triangles, assign point data according to
  // the y-value of the point, convert the point data to cell data (treating the
  // data as categorical), and compare the results to an established truth
  // array.

  svtkSmartPointer<svtkUnstructuredGrid> unstructuredGrid =
    svtkSmartPointer<svtkUnstructuredGrid>::New();

  svtkSmartPointer<svtkPoints> pointArray = svtkSmartPointer<svtkPoints>::New();

  svtkSmartPointer<svtkPointLocator> pointLocator = svtkSmartPointer<svtkPointLocator>::New();
  double bounds[6] = { -1., 1., -1., 1., -1., 1. };
  pointLocator->InitPointInsertion(pointArray, bounds);

  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();

  // Our triangle grid is comprised of a 4 x 4 grid of squares, with each square
  // cut along the diagonal.
  svtkIdType nX = 4;
  svtkIdType nY = 4;

  double p[4][3];
  double dx = (bounds[1] - bounds[0]) / nX;
  double dy = (bounds[3] - bounds[2]) / nY;
  for (svtkIdType i = 0; i < 4; i++)
  {
    for (svtkIdType j = 0; j < 2; j++)
    {
      p[i][j] = bounds[2 * j];
    }
    p[i][2] = 0.;
  }
  p[1][0] += dx;
  p[2][0] += dx;
  p[2][1] += dy;
  p[3][1] += dy;

  for (svtkIdType xInc = 0; xInc < nX; xInc++)
  {
    p[0][1] = p[1][1] = bounds[2];
    p[2][1] = p[3][1] = bounds[2] + dy;

    for (svtkIdType yInc = 0; yInc < nY; yInc++)
    {
      if ((xInc < nX / 2) == (yInc < nY / 2))
      {
        AddTriangle(p[0], p[1], p[3], pointLocator, cellArray);
        AddTriangle(p[1], p[2], p[3], pointLocator, cellArray);
      }
      else
      {
        AddTriangle(p[0], p[1], p[2], pointLocator, cellArray);
        AddTriangle(p[0], p[2], p[3], pointLocator, cellArray);
      }
      for (svtkIdType i = 0; i < 4; i++)
      {
        p[i][1] += dy;
      }
    }

    p[0][0] = p[3][0] = p[1][0];
    p[1][0] += dx;
    p[2][0] += dx;
  }

  unstructuredGrid->SetPoints(pointArray);
  unstructuredGrid->SetCells(SVTK_TRIANGLE, cellArray);

  svtkIdType nPoints = unstructuredGrid->GetPoints()->GetNumberOfPoints();

  // Construct elevation point data by assigning each point its own y-value.
  svtkSmartPointer<svtkDoubleArray> elevation = svtkSmartPointer<svtkDoubleArray>::New();
  elevation->SetName("Elevation");
  elevation->SetNumberOfTuples(nPoints);

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    double xyz[3];
    unstructuredGrid->GetPoints()->GetPoint(i, xyz);
    elevation->SetTypedTuple(i, &xyz[1]);
  }

  unstructuredGrid->GetPointData()->AddArray(elevation);
  unstructuredGrid->GetPointData()->SetScalars(elevation);

  // Convert point data to cell data, treating the data as categorical.
  svtkSmartPointer<svtkPointDataToCellData> pointDataToCellData =
    svtkSmartPointer<svtkPointDataToCellData>::New();
  pointDataToCellData->SetInputData(unstructuredGrid);
  pointDataToCellData->SetCategoricalData(true);
  pointDataToCellData->Update();

  // Test the output.
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(pointDataToCellData->GetOutput());

  if (!output)
  {
    return EXIT_FAILURE;
  }

  svtkDoubleArray* cellElevation =
    svtkDoubleArray::SafeDownCast(output->GetCellData()->GetScalars("Elevation"));

  if (!cellElevation)
  {
    return EXIT_FAILURE;
  }

  double expectedCellElevationValues[8] = { -1., -.5, -.5, 0., 0., .5, .5, 1. };

  for (svtkIdType i = 0; i < cellElevation->GetNumberOfTuples(); i++)
  {
    if (std::fabs(expectedCellElevationValues[i % 8] - cellElevation->GetTuple1(i)) > EPSILON)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
