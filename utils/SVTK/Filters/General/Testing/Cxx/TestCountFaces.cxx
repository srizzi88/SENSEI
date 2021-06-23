/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCountFaces.h"

#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkUnstructuredGrid.h"

int TestCountFaces(int, char*[])
{
  svtkNew<svtkUnstructuredGrid> data;
  svtkNew<svtkPoints> points;
  svtkNew<svtkIdList> cell;
  svtkNew<svtkCountFaces> filter;

  // Need 12 points to test all cell types:
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(0., 0., 0.);
  }
  data->SetPoints(points);

  // Insert the following cell types and verify the number of faces computed
  // by the filter:
  // SVTK_VERTEX = 0
  // SVTK_LINE = 0
  // SVTK_TRIANGLE = 0
  // SVTK_TETRA = 4
  // SVTK_PYRAMID = 5
  // SVTK_WEDGE = 5
  // SVTK_VOXEL = 6
  // SVTK_HEXAHEDRON = 6
  // SVTK_PENTAGONAL_PRISM = 7
  // SVTK_HEXAGONAL_PRISM = 8

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

  svtkIdTypeArray* faces =
    svtkIdTypeArray::SafeDownCast(output->GetCellData()->GetArray(filter->GetOutputArrayName()));
  if (!faces)
  {
    std::cerr << "No output array!\n";
    return EXIT_FAILURE;
  }

  if (faces->GetNumberOfComponents() != 1)
  {
    std::cerr << "Invalid number of components in output array: " << faces->GetNumberOfComponents()
              << "\n";
    return EXIT_FAILURE;
  }

  if (faces->GetNumberOfTuples() != 10)
  {
    std::cerr << "Invalid number of components in output array: " << faces->GetNumberOfTuples()
              << "\n";
    return EXIT_FAILURE;
  }

#define TEST_FACES(idx, expected)                                                                  \
  {                                                                                                \
    svtkIdType numFaces = faces->GetTypedComponent(idx, 0);                                         \
    if (numFaces != (expected))                                                                    \
    {                                                                                              \
      std::cerr << "Expected cell @idx=" << (idx) << " to have " << (expected)                     \
                << " faces, but found " << numFaces << "\n";                                       \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  }

  int idx = 0;
  // SVTK_VERTEX = 0
  TEST_FACES(idx++, 0);
  // SVTK_LINE = 0
  TEST_FACES(idx++, 0);
  // SVTK_TRIANGLE = 0
  TEST_FACES(idx++, 0);
  // SVTK_TETRA = 4
  TEST_FACES(idx++, 4);
  // SVTK_PYRAMID = 5
  TEST_FACES(idx++, 5);
  // SVTK_WEDGE = 5
  TEST_FACES(idx++, 5);
  // SVTK_VOXEL = 6
  TEST_FACES(idx++, 6);
  // SVTK_HEXAHEDRON = 6
  TEST_FACES(idx++, 6);
  // SVTK_PENTAGONAL_PRISM = 7
  TEST_FACES(idx++, 7);
  // SVTK_HEXAGONAL_PRISM = 8
  TEST_FACES(idx++, 8);

#undef TEST_FACES

  return EXIT_SUCCESS;
}
