/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceFilterWith1DGrids.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

#include <cstdlib>

svtkSmartPointer<svtkDataSet> CreateRectilinearGrid()
{
  svtkSmartPointer<svtkRectilinearGrid> grid = svtkSmartPointer<svtkRectilinearGrid>::New();
  grid->SetDimensions(10, 1, 1);

  svtkSmartPointer<svtkDoubleArray> xArray = svtkSmartPointer<svtkDoubleArray>::New();
  for (int x = 0; x < 10; x++)
  {
    xArray->InsertNextValue(x);
  }

  svtkSmartPointer<svtkDoubleArray> yArray = svtkSmartPointer<svtkDoubleArray>::New();
  yArray->InsertNextValue(0.0);

  svtkSmartPointer<svtkDoubleArray> zArray = svtkSmartPointer<svtkDoubleArray>::New();
  zArray->InsertNextValue(0.0);

  grid->SetXCoordinates(xArray);
  grid->SetYCoordinates(yArray);
  grid->SetZCoordinates(zArray);

  return grid;
}

svtkSmartPointer<svtkDataSet> CreateStructuredGrid()
{
  svtkSmartPointer<svtkStructuredGrid> grid = svtkSmartPointer<svtkStructuredGrid>::New();

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  for (int x = 0; x < 10; x++)
  {
    points->InsertNextPoint(x, 0., 0.);
  }

  // Specify the dimensions of the grid
  grid->SetDimensions(10, 1, 1);
  grid->SetPoints(points);

  return grid;
}

int TestSurfaceFilter(svtkDataSet* grid)
{
  svtkSmartPointer<svtkDataSetSurfaceFilter> surfaceFilter =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surfaceFilter->SetInputData(grid);
  surfaceFilter->Update();

  svtkPolyData* surface = surfaceFilter->GetOutput();
  int numCells = surface->GetNumberOfCells();
  if (numCells != 9)
  {
    std::cerr << "Expected 9 cells, got: " << numCells << std::endl;
    return EXIT_FAILURE;
  }
  for (int i = 0; i < numCells; i++)
  {
    if (surface->GetCellType(i) != SVTK_LINE)
    {
      std::cerr << "Cell " << i << " is not a SVTK_LINE!" << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestDataSetSurfaceFilterWith1DGrids(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int ret = 0;

  ret |= TestSurfaceFilter(CreateRectilinearGrid());

  ret |= TestSurfaceFilter(CreateStructuredGrid());

  return ret;
}
