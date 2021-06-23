/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderRelative.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkOBJReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkOBJReader.h"

#include "svtkCellArray.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

//-----------------------------------------------------------------------------
int CheckArrayPointData(svtkDataArray* firstArray, svtkDataArray* secondArray, int idx)
{
  // Check that each component at a given index are the same in each array
  for (int compIdx = 0; compIdx < secondArray->GetNumberOfComponents(); ++compIdx)
  {
    if (firstArray->GetComponent(idx, compIdx) != secondArray->GetComponent(idx, compIdx))
    {
      cerr << "Error: different values for "
              "["
           << (idx) << "]_" << compIdx << endl;
      return 1;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
int TestOBJReaderRelative(int argc, char* argv[])
{
  int retVal = 0;

  // Create the reader.
  char* fname_rel = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/relative_indices.obj");
  svtkSmartPointer<svtkOBJReader> reader_rel = svtkSmartPointer<svtkOBJReader>::New();
  reader_rel->SetFileName(fname_rel);
  reader_rel->Update();
  delete[] fname_rel;

  // Create the reader.
  char* fname_abs = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/absolute_indices.obj");
  svtkSmartPointer<svtkOBJReader> reader_abs = svtkSmartPointer<svtkOBJReader>::New();
  reader_abs->SetFileName(fname_abs);
  reader_abs->Update();
  delete[] fname_abs;

  svtkPolyData* data_rel = reader_rel->GetOutput();
  svtkPolyData* data_abs = reader_abs->GetOutput();

#define CHECK(obj, method)                                                                         \
  if (obj##_rel->method != obj##_abs->method)                                                      \
  {                                                                                                \
    cerr << "Error: different values for " #obj "->" #method << endl;                              \
    retVal = 1;                                                                                    \
  }
#define CHECK_ARRAY(obj, idx)                                                                      \
  if (obj##_rel[idx] != obj##_abs[idx])                                                            \
  {                                                                                                \
    cerr << "Error: different values for " #obj "[" << (idx) << "]" << endl;                       \
    retVal = 1;                                                                                    \
  }
#define CHECK_SCALAR(obj)                                                                          \
  if (obj##_rel != obj##_abs)                                                                      \
  {                                                                                                \
    cerr << "Error: different values for " #obj << endl;                                           \
    retVal = 1;                                                                                    \
  }

#define CHECK_ARRAY_EXISTS(array)                                                                  \
  if (!(array))                                                                                    \
  {                                                                                                \
    cerr << "Array does not exist." << endl;                                                       \
    retVal = 1;                                                                                    \
  }

  CHECK(data, GetNumberOfVerts())
  CHECK(data, GetNumberOfLines())
  CHECK(data, GetNumberOfCells())
  CHECK(data, GetNumberOfStrips())

  svtkCellArray* polys_rel = data_rel->GetPolys();
  svtkCellArray* polys_abs = data_abs->GetPolys();

  CHECK(polys, GetNumberOfCells());

  svtkIdType npts_rel;
  svtkIdType npts_abs;
  const svtkIdType* pts_rel;
  const svtkIdType* pts_abs;

  polys_rel->InitTraversal();
  polys_abs->InitTraversal();

  // Get the texture and normal arrays to check
  svtkDataArray* tcoords_rel = data_rel->GetPointData()->GetTCoords();
  svtkDataArray* tcoords_abs = data_abs->GetPointData()->GetTCoords();

  CHECK_ARRAY_EXISTS(tcoords_rel)
  CHECK_ARRAY_EXISTS(tcoords_abs)

  int tcoordsNbComp_rel = tcoords_rel->GetNumberOfComponents();
  int tcoordsNbComp_abs = tcoords_abs->GetNumberOfComponents();

  svtkDataArray* normals_rel = data_rel->GetPointData()->GetNormals();
  svtkDataArray* normals_abs = data_abs->GetPointData()->GetNormals();

  CHECK_ARRAY_EXISTS(normals_rel)
  CHECK_ARRAY_EXISTS(normals_abs)

  int normalsNbComp_rel = normals_rel->GetNumberOfComponents();
  int normalsNbComp_abs = normals_abs->GetNumberOfComponents();

  CHECK_SCALAR(tcoordsNbComp)
  CHECK_SCALAR(normalsNbComp)

  while (polys_rel->GetNextCell(npts_rel, pts_rel) && polys_abs->GetNextCell(npts_abs, pts_abs))
  {
    CHECK_SCALAR(npts)

    for (svtkIdType i = 0; i < npts_rel && i < npts_abs; ++i)
    {
      CHECK_ARRAY(pts, i)

      // For each points, check if the point data associated with the points
      // from the OBJ using relative coordinates matches the ones from the
      // OBJ using absolute coordinates
      retVal = CheckArrayPointData(tcoords_rel, tcoords_abs, i) ||
        CheckArrayPointData(normals_rel, normals_abs, i);
    }
  }

#undef CHECK_ARRAY_EXISTS
#undef CHECK_SCALAR
#undef CHECK_ARRAY
#undef CHECK

  return retVal;
}
