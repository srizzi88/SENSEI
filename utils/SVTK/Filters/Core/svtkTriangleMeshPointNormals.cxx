/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTriangleMeshPointNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTriangleMeshPointNormals.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkTriangleMeshPointNormals);

namespace
{

struct ComputeNormalsDirection
{
  template <typename ArrayT>
  void operator()(ArrayT *pointArray,
                  svtkPolyData *mesh,
                  svtkFloatArray *normalsArray)
  {
    const auto points = svtk::DataArrayTupleRange<3>(pointArray);
    auto normals = svtk::DataArrayTupleRange<3>(normalsArray);

    float a[3], b[3], tn[3];

    auto cellIter = svtk::TakeSmartPointer(mesh->GetPolys()->NewIterator());
    for (cellIter->GoToFirstCell();
         !cellIter->IsDoneWithTraversal();
         cellIter->GoToNextCell())
    {
      svtkIdType cellSize;
      const svtkIdType *cell;
      cellIter->GetCurrentCell(cellSize, cell);

      // First value in cellArray indicates number of points in cell.
      // We need 3 to compute normals.
      if (cellSize == 3)
      {
        const auto p0 = points[cell[0]];
        const auto p1 = points[cell[1]];
        const auto p2 = points[cell[2]];
        auto n0 = normals[cell[0]];
        auto n1 = normals[cell[1]];
        auto n2 = normals[cell[2]];

        // two vectors
        a[0] = static_cast<float>(p2[0] - p1[0]);
        a[1] = static_cast<float>(p2[1] - p1[1]);
        a[2] = static_cast<float>(p2[2] - p1[2]);
        b[0] = static_cast<float>(p0[0] - p1[0]);
        b[1] = static_cast<float>(p0[1] - p1[1]);
        b[2] = static_cast<float>(p0[2] - p1[2]);

        // cell normals by cross-product
        // (no need to normalize those + it's faster not to)
        tn[0] = (a[1] * b[2] - a[2] * b[1]);
        tn[1] = (a[2] * b[0] - a[0] * b[2]);
        tn[2] = (a[0] * b[1] - a[1] * b[0]);

        // append triangle normals to point normals
        n0[0] += tn[0];
        n0[1] += tn[1];
        n0[2] += tn[2];
        n1[0] += tn[0];
        n1[1] += tn[1];
        n1[2] += tn[2];
        n2[0] += tn[0];
        n2[1] += tn[1];
        n2[2] += tn[2];
      }
      // If degenerate cell
      else if (cellSize < 3)
      {
        svtkGenericWarningMacro(
              "Some cells are degenerate (less than 3 points). "
              "Use svtkCleanPolyData beforehand to correct this.");
        return;
      }
      // If cell not triangle
      else
      {
        svtkGenericWarningMacro(
              "Some cells have too many points (more than 3 points). "
              "Use svtkTriangulate to correct this.");
        return;
      }
    }
  }
};

} // end anon namespace

// Generate normals for polygon meshes
int svtkTriangleMeshPointNormals::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Generating surface normals");

  svtkIdType numPts = input->GetNumberOfPoints(); // nbr of points from input
  if (numPts < 1)
  {
    svtkDebugMacro(<< "No data to generate normals for!");
    return 1;
  }

  if (input->GetVerts()->GetNumberOfCells() != 0 || input->GetLines()->GetNumberOfCells() != 0 ||
    input->GetStrips()->GetNumberOfCells() != 0)
  {
    svtkErrorMacro(
      << "Can not compute normals for a mesh with Verts, Lines or Strips, as it will "
      << "corrupt the number of points used during the normals computation."
      << "Make sure your input PolyData only has triangles (Polys with 3 components)).");
    return 0;
  }

  // Copy structure and cell data
  output->CopyStructure(input);
  output->GetCellData()->PassData(input->GetCellData());

  // If there is nothing to do, pass the point data through
  if (input->GetNumberOfPolys() < 1)
  {
    output->GetPointData()->PassData(input->GetPointData());
    return 1;
  }
  // Else pass everything but normals
  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());

  // Prepare array for normals
  svtkFloatArray* normals = svtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);
  normals->SetName("Normals");
  normals->FillValue(0.0);
  output->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->UpdateProgress(0.1);

  // Fast-path for float/double points:
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::DispatchByValueType<Reals>;
  ComputeNormalsDirection worker;

  svtkDataArray *points = output->GetPoints()->GetData();
  if (!Dispatcher::Execute(points, worker, output, normals))
  { // fallback for integral point arrays
    worker(points, output, normals);
  }

  this->UpdateProgress(0.5);

  // Normalize point normals
  float l;
  unsigned int i3;
  float *n = normals->GetPointer(0);
  for (svtkIdType i = 0; i < numPts; ++i)
  {
    i3 = i * 3;
    if ((l = sqrt(n[i3] * n[i3] + n[i3 + 1] * n[i3 + 1] + n[i3 + 2] * n[i3 + 2])) != 0.0)
    {
      n[i3] /= l;
      n[i3 + 1] /= l;
      n[i3 + 2] /= l;
    }
  }
  this->UpdateProgress(0.9);

  // Update modified time
  normals->Modified();

  return 1;
}

void svtkTriangleMeshPointNormals::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
