/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCountVertices.h"

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkUnstructuredGrid.h"

int TestCountVertices(int, char*[])
{
  svtkNew<svtkUnstructuredGrid> data;
  svtkNew<svtkPoints> points;
  svtkNew<svtkIdList> cell;
  svtkNew<svtkCountVertices> filter;

  // Need 12 points to test all cell types:
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(0., 0., 0.);
  }
  data->SetPoints(points);

  // Insert the following cell types and verify the number of verts computed
  // by the filter:
  // SVTK_VERTEX = 1
  // SVTK_LINE = 2
  // SVTK_TRIANGLE = 3
  // SVTK_TETRA = 4
  // SVTK_PYRAMID = 5
  // SVTK_WEDGE = 6
  // SVTK_VOXEL = 8
  // SVTK_HEXAHEDRON = 8
  // SVTK_PENTAGONAL_PRISM = 10
  // SVTK_HEXAGONAL_PRISM = 12

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_VERTEX, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_LINE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_TRIANGLE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_TETRA, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_PYRAMID, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_WEDGE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_VOXEL, cell);
  data->InsertNextCell(SVTK_HEXAHEDRON, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_PENTAGONAL_PRISM, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(SVTK_HEXAGONAL_PRISM, cell);

  filter->SetInputData(data);
  filter->Update();

  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(filter->GetOutput());
  if (!output)
  {
    std::cerr << "No output data!\n";
    return EXIT_FAILURE;
  }

  svtkIdTypeArray* verts =
    svtkIdTypeArray::SafeDownCast(output->GetCellData()->GetArray(filter->GetOutputArrayName()));
  if (!verts)
  {
    std::cerr << "No output array!\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfComponents() != 1)
  {
    std::cerr << "Invalid number of components in output array: " << verts->GetNumberOfComponents()
              << "\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfTuples() != 10)
  {
    std::cerr << "Invalid number of components in output array: " << verts->GetNumberOfTuples()
              << "\n";
    return EXIT_FAILURE;
  }

#define TEST_VERTICES(idx, expected)                                                               \
  {                                                                                                \
    svtkIdType numVerts = verts->GetTypedComponent(idx, 0);                                         \
    if (numVerts != (expected))                                                                    \
    {                                                                                              \
      std::cerr << "Expected cell @idx=" << (idx) << " to have " << (expected)                     \
                << " vertices, but found " << numVerts << "\n";                                    \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  }

  int idx = 0;
  // SVTK_VERTEX = 1
  TEST_VERTICES(idx++, 1);
  // SVTK_LINE = 2
  TEST_VERTICES(idx++, 2);
  // SVTK_TRIANGLE = 3
  TEST_VERTICES(idx++, 3);
  // SVTK_TETRA = 4
  TEST_VERTICES(idx++, 4);
  // SVTK_PYRAMID = 5
  TEST_VERTICES(idx++, 5);
  // SVTK_WEDGE = 6
  TEST_VERTICES(idx++, 6);
  // SVTK_VOXEL = 8
  TEST_VERTICES(idx++, 8);
  // SVTK_HEXAHEDRON = 8
  TEST_VERTICES(idx++, 8);
  // SVTK_PENTAGONAL_PRISM = 10
  TEST_VERTICES(idx++, 10);
  // SVTK_HEXAGONAL_PRISM = 12
  TEST_VERTICES(idx++, 12);

#undef TEST_VERTICES

  return EXIT_SUCCESS;
}
