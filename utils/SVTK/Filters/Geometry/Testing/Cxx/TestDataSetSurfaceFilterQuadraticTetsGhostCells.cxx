/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceFilterQuadraticTetsGhostCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstdlib>

#include "svtkDataSetSurfaceFilter.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"

int TestDataSetSurfaceFilterQuadraticTetsGhostCells(int argc, char* argv[])
{
  char* cfname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadratic_tets_with_ghost_cells_0.vtu");

  svtkNew<svtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(cfname);
  delete[] cfname;

  svtkNew<svtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputConnection(reader->GetOutputPort());
  surfaceFilter->Update();

  svtkPolyData* surface = surfaceFilter->GetOutput();
  int numCells = surface->GetNumberOfCells();
  if (numCells != 672)
  {
    std::cerr << "Expected 672 cells, got: " << numCells << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
