/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkRectilinearGridToPointSet.h>

#include <svtkDoubleArray.h>
#include <svtkMath.h>
#include <svtkRectilinearGrid.h>
#include <svtkStructuredGrid.h>

#include <svtkNew.h>
#include <svtkSmartPointer.h>

#include <ctime>

static svtkSmartPointer<svtkDataArray> MonotonicValues(svtkIdType numValues)
{
  svtkSmartPointer<svtkDoubleArray> values = svtkSmartPointer<svtkDoubleArray>::New();
  values->SetNumberOfComponents(1);
  values->SetNumberOfTuples(numValues);

  double v = svtkMath::Random();
  for (svtkIdType id = 0; id < numValues; id++)
  {
    values->SetValue(id, v);
    v += svtkMath::Random();
  }

  return values;
}

static svtkSmartPointer<svtkRectilinearGrid> MakeRectilinearGrid()
{
  svtkSmartPointer<svtkRectilinearGrid> grid = svtkSmartPointer<svtkRectilinearGrid>::New();

  int extent[6];
  for (int i = 0; i < 6; i += 2)
  {
    extent[i] = std::lround(svtkMath::Random(-10, 10));
    extent[i + 1] = extent[i] + std::lround(svtkMath::Random(0, 10));
  }

  grid->SetExtent(extent);

  svtkSmartPointer<svtkDataArray> coord;

  coord = MonotonicValues(extent[1] - extent[0] + 1);
  grid->SetXCoordinates(coord);

  coord = MonotonicValues(extent[3] - extent[2] + 1);
  grid->SetYCoordinates(coord);

  coord = MonotonicValues(extent[5] - extent[4] + 1);
  grid->SetZCoordinates(coord);

  return grid;
}

int TestRectilinearGridToPointSet(int, char*[])
{
  int seed = time(nullptr);
  std::cout << "Seed: " << seed << std::endl;
  svtkMath::RandomSeed(seed);

  svtkSmartPointer<svtkRectilinearGrid> inData = MakeRectilinearGrid();

  svtkNew<svtkRectilinearGridToPointSet> rect2points;
  rect2points->SetInputData(inData);
  rect2points->Update();

  svtkDataSet* outData = rect2points->GetOutput();

  svtkIdType numPoints = inData->GetNumberOfPoints();
  if (numPoints != outData->GetNumberOfPoints())
  {
    std::cout << "Got wrong number of points: " << numPoints << " vs "
              << outData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  svtkIdType numCells = inData->GetNumberOfCells();
  if (numCells != outData->GetNumberOfCells())
  {
    std::cout << "Got wrong number of cells: " << numCells << " vs " << outData->GetNumberOfCells()
              << std::endl;
    return EXIT_FAILURE;
  }

  for (svtkIdType pointId = 0; pointId < numPoints; pointId++)
  {
    double inPoint[3];
    double outPoint[3];

    inData->GetPoint(pointId, inPoint);
    outData->GetPoint(pointId, outPoint);

    if ((inPoint[0] != outPoint[0]) || (inPoint[1] != outPoint[1]) || (inPoint[2] != outPoint[2]))
    {
      std::cout << "Got mismatched point coordinates." << std::endl;
      std::cout << "Input: " << inPoint[0] << " " << inPoint[1] << " " << inPoint[2] << std::endl;
      std::cout << "Output: " << outPoint[0] << " " << outPoint[1] << " " << outPoint[2]
                << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
