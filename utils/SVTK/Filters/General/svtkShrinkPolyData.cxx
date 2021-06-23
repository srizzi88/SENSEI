/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShrinkPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkShrinkPolyData.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkShrinkPolyData);

svtkShrinkPolyData::svtkShrinkPolyData(double sf)
{
  sf = (sf < 0.0 ? 0.0 : (sf > 1.0 ? 1.0 : sf));
  this->ShrinkFactor = sf;
}

namespace
{

struct ShrinkWorker
{
  template <typename PointArrayT>
  void operator()(PointArrayT* inPtArray, svtkShrinkPolyData* self, double shrinkFactor,
    svtkInformation* inInfo, svtkInformation* outInfo)
  {
    using T = svtk::GetAPIType<PointArrayT>;

    int j, k;
    T center[3];
    int abortExecute = 0;
    svtkCellArray *newVerts, *newLines, *newPolys;
    svtkPointData* pd;
    svtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
    svtkIdType numNewPts, numNewLines, numNewPolys, polyAllocSize;
    svtkIdType npts = 0;
    const svtkIdType* pts = nullptr;
    svtkIdType newIds[3] = { 0, 0, 0 };
    svtkPoints* newPoints;
    svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
    svtkPointData* pointData = output->GetPointData();

    pd = input->GetPointData();

    inVerts = input->GetVerts();
    inLines = input->GetLines();
    inPolys = input->GetPolys();
    inStrips = input->GetStrips();

    // Count the number of new points and other primitives that
    // need to be created.
    //
    numNewPts = input->GetNumberOfVerts();
    numNewLines = 0;
    numNewPolys = 0;
    polyAllocSize = 0;

    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts);)
    {
      numNewPts += (npts - 1) * 2;
      numNewLines += npts - 1;
    }
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
    {
      numNewPts += npts;
      numNewPolys++;
      polyAllocSize += npts + 1;
    }
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      numNewPts += (npts - 2) * 3;
      polyAllocSize += (npts - 2) * 4;
    }

    // Allocate
    //
    newVerts = svtkCellArray::New();
    newVerts->AllocateCopy(input->GetVerts());

    newLines = svtkCellArray::New();
    newLines->AllocateEstimate(numNewLines, 2);

    newPolys = svtkCellArray::New();
    newPolys->AllocateEstimate(polyAllocSize, 1);

    pointData->CopyAllocate(pd);

    auto newPointsArray = svtk::TakeSmartPointer(inPtArray->NewInstance());
    newPointsArray->SetNumberOfComponents(3);
    newPoints = input->GetPoints()->NewInstance();
    newPoints->SetData(newPointsArray);
    newPoints->Allocate(numNewPts);
    newPoints->SetNumberOfPoints(numNewPts);
    svtkIdType outCount = 0;

    const auto inPts = svtk::DataArrayTupleRange<3>(inPtArray);
    auto outPts = svtk::DataArrayTupleRange<3>(newPointsArray);

    // Copy vertices (no shrinking necessary)
    //
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts) && !abortExecute;)
    {
      newVerts->InsertNextCell(npts);
      for (j = 0; j < npts; j++)
      {
        outPts[outCount] = inPts[pts[j]];

        newVerts->InsertCellPoint(outCount);
        pointData->CopyData(pd, pts[j], outCount);
        outCount++;
      }
      abortExecute = self->GetAbortExecute();
    }
    self->UpdateProgress(0.10);

    // Lines need to be shrunk, and if polyline, split into separate pieces
    //
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts) && !abortExecute;)
    {
      for (j = 0; j < (npts - 1); j++)
      {
        const auto p1 = inPts[pts[j]];
        const auto p2 = inPts[pts[j + 1]];
        for (k = 0; k < 3; k++)
        {
          center[k] = (p1[k] + p2[k]) / 2;
        }

        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p1[k] - center[k]));
        }
        pointData->CopyData(pd, pts[j], outCount);
        outCount++;

        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p2[k] - center[k]));
        }
        pointData->CopyData(pd, pts[j + 1], outCount);
        newIds[0] = outCount - 1;
        newIds[1] = outCount;
        newLines->InsertNextCell(2, newIds);
        outCount++;
      }
      abortExecute = self->GetAbortExecute();
    }
    self->UpdateProgress(0.25);

    // Polygons need to be shrunk
    //
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts) && !abortExecute;)
    {
      for (center[0] = center[1] = center[2] = 0, j = 0; j < npts; j++)
      {
        const auto p1 = inPts[pts[j]];
        for (k = 0; k < 3; k++)
        {
          center[k] += p1[k];
        }
      }

      for (k = 0; k < 3; k++)
      {
        center[k] /= npts;
      }

      newPolys->InsertNextCell(npts);
      for (j = 0; j < npts; j++)
      {
        const auto p1 = inPts[pts[j]];
        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p1[k] - center[k]));
        }
        newPolys->InsertCellPoint(outCount);
        pointData->CopyData(pd, pts[j], outCount);
        outCount++;
      }
      abortExecute = self->GetAbortExecute();
    }
    self->UpdateProgress(0.75);

    // Triangle strips need to be shrunk and split into separate pieces.
    //
    svtkIdType tmp;
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts) && !abortExecute;)
    {
      for (j = 0; j < (npts - 2); j++)
      {
        const auto p1 = inPts[pts[j]];
        const auto p2 = inPts[pts[j + 1]];
        const auto p3 = inPts[pts[j + 2]];

        for (k = 0; k < 3; k++)
        {
          center[k] = (p1[k] + p2[k] + p3[k]) / 3;
        }

        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p1[k] - center[k]));
        }
        pointData->CopyData(pd, pts[j], outCount);
        newIds[0] = outCount;
        outCount++;

        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p2[k] - center[k]));
        }
        pointData->CopyData(pd, pts[j + 1], outCount);
        newIds[1] = outCount;
        outCount++;

        for (k = 0; k < 3; k++)
        {
          outPts[outCount][k] = static_cast<T>(center[k] + shrinkFactor * (p3[k] - center[k]));
        }
        pointData->CopyData(pd, pts[j + 2], outCount);
        newIds[1] = outCount;
        outCount++;

        // must reverse order for every other triangle
        if (j % 2)
        {
          tmp = newIds[0];
          newIds[0] = newIds[2];
          newIds[2] = tmp;
        }
        newPolys->InsertNextCell(3, newIds);
      }
      abortExecute = self->GetAbortExecute();
    }

    assert(outCount == numNewPts);

    // Update self and release memory
    //
    output->SetPoints(newPoints);
    newPoints->Delete();

    output->SetVerts(newVerts);
    newVerts->Delete();

    output->SetLines(newLines);
    newLines->Delete();

    output->SetPolys(newPolys);
    newPolys->Delete();

    output->GetCellData()->PassData(input->GetCellData());
  }
};

} // end anon namespace

int svtkShrinkPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Initialize
  svtkDebugMacro(<< "Shrinking polygonal data");

  if (input == nullptr || input->GetPoints() == nullptr)
  {
    return 1;
  }

  // Use a fast-path for float/double points
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::DispatchByValueType<Reals>;
  ShrinkWorker worker;
  if (!Dispatcher::Execute(
        input->GetPoints()->GetData(), worker, this, this->ShrinkFactor, inInfo, outInfo))
  { // Fallback to slowpath for other array types:
    worker(input->GetPoints()->GetData(), this, this->ShrinkFactor, inInfo, outInfo);
  }

  return 1;
}

void svtkShrinkPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
