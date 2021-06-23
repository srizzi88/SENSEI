/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointCloudFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointCloudFilter.h"

#include "svtkAbstractPointLocator.h"
#include "svtkArrayDispatch.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// Map input points to output. Basically the third pass of the algorithm.
struct MapPoints
{
  template <typename InPointsT, typename OutPointsT>
  void operator()(InPointsT* inPointsArray, OutPointsT* outPointsArray, svtkIdType* map,
    svtkPointData* inPD, svtkPointData* outPD)
  {
    const auto inPts = svtk::DataArrayTupleRange<3>(inPointsArray);
    auto outPts = svtk::DataArrayTupleRange<3>(outPointsArray);

    ArrayList arrays;
    arrays.AddArrays(outPts.size(), inPD, outPD, 0.0, false);

    svtkSMPTools::For(0, inPts.size(), [&](svtkIdType ptId, svtkIdType endPtId) {
      for (; ptId < endPtId; ++ptId)
      {
        const svtkIdType outPtId = map[ptId];
        if (outPtId != -1)
        {
          outPts[outPtId] = inPts[ptId];
          arrays.Copy(ptId, outPtId);
        }
      }
    });
  }
};

//----------------------------------------------------------------------------
// Map outlier points to second output. This is an optional pass of the
// algorithm.
struct MapOutliers
{
  template <typename InPointsT, typename OutPointsT>
  void operator()(InPointsT* inPtArray, OutPointsT* outPtArray, svtkIdType* map, svtkPointData* inPD,
    svtkPointData* outPD)
  {
    const auto inPts = svtk::DataArrayTupleRange<3>(inPtArray);
    auto outPts = svtk::DataArrayTupleRange<3>(outPtArray);

    ArrayList arrays;
    arrays.AddArrays(outPts.size(), inPD, outPD, 0.0, false);

    svtkSMPTools::For(0, inPts.size(), [&](svtkIdType ptId, svtkIdType endPtId) {
      for (; ptId < endPtId; ++ptId)
      {
        svtkIdType outPtId = map[ptId];
        if (outPtId < 0)
        {
          outPtId = (-outPtId) - 1;
          outPts[outPtId] = inPts[ptId];
          arrays.Copy(ptId, outPtId);
        }
      }
    });
  }
}; // MapOutliers

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkPointCloudFilter::svtkPointCloudFilter()
{
  this->PointMap = nullptr;
  this->NumberOfPointsRemoved = 0;
  this->GenerateOutliers = false;
  this->GenerateVertices = false;

  // Optional second output of outliers
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
svtkPointCloudFilter::~svtkPointCloudFilter()
{
  delete[] this->PointMap;
}

//----------------------------------------------------------------------------
const svtkIdType* svtkPointCloudFilter::GetPointMap()
{
  return this->PointMap;
}

//----------------------------------------------------------------------------
svtkIdType svtkPointCloudFilter::GetNumberOfPointsRemoved()
{
  return this->NumberOfPointsRemoved;
}

//----------------------------------------------------------------------------
// There are three high level passes. First we traverse all the input points
// to see how many neighbors each point has within a specified radius, and a
// map is created indicating whether an input point is to be copied to the
// output. Next a prefix sum is used to count the output points, and to
// update the mapping between the input and the output. Finally, non-removed
// input points (and associated attributes) are copied to the output.
int svtkPointCloudFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Reset the filter
  this->NumberOfPointsRemoved = 0;

  delete[] this->PointMap; // might have executed previously

  // Check input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Okay invoke filtering operation. This is always the initial pass.
  this->PointMap = new svtkIdType[numPts];
  if (!this->FilterPoints(input))
  {
    return 1;
  }

  // Count the resulting points (prefix sum). The second pass of the algorithm; it
  // could be threaded but prefix sum does not benefit very much from threading.
  svtkIdType ptId;
  svtkIdType count = 0;
  svtkIdType* map = this->PointMap;
  for (ptId = 0; ptId < numPts; ++ptId)
  {
    if (map[ptId] != -1)
    {
      map[ptId] = count;
      count++;
    }
  }
  this->NumberOfPointsRemoved = numPts - count;

  // If the number of input and output points is the same we short circuit
  // the process. Otherwise, copy the masked input points to the output.
  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  if (this->NumberOfPointsRemoved == 0)
  {
    output->SetPoints(input->GetPoints());
    outPD->PassData(inPD);
    this->GenerateVerticesIfRequested(output);

    return 1;
  }

  // Okay copy the points from the input to the output. We use a threaded
  // operation that provides a minor benefit (since it's mostly data
  // movement with almost no computation).
  outPD->CopyAllocate(inPD, count);
  svtkPoints* points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->SetNumberOfPoints(count);
  output->SetPoints(points);

  // Use fast path for float/double points:
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::Dispatch2BySameValueType<Reals>;
  MapPoints worker;
  svtkDataArray* inPtArray = input->GetPoints()->GetData();
  svtkDataArray* outPtArray = output->GetPoints()->GetData();
  if (!Dispatcher::Execute(inPtArray, outPtArray, worker, this->PointMap, inPD, outPD))
  { // fallback for weird types:
    worker(inPtArray, outPtArray, this->PointMap, inPD, outPD);
  }

  // Generate poly vertex cell if requested
  this->GenerateVerticesIfRequested(output);

  // Clean up. We leave the map in case the user wants to use it.
  points->Delete();

  // Create the second output if requested. Note that we are using a negative
  // count in the map (offset by -1) which indicates the final position of
  // the output point in the second output.
  if (this->GenerateOutliers && this->NumberOfPointsRemoved > 0)
  {
    svtkInformation* outInfo2 = outputVector->GetInformationObject(1);
    // get the second output
    svtkPolyData* output2 = svtkPolyData::SafeDownCast(outInfo2->Get(svtkDataObject::DATA_OBJECT()));
    svtkPointData* outPD2 = output2->GetPointData();
    outPD2->CopyAllocate(inPD, (count - 1));

    // Update map
    count = 1; // offset by one
    map = this->PointMap;
    for (ptId = 0; ptId < numPts; ++ptId)
    {
      if (map[ptId] == -1)
      {
        map[ptId] = (-count);
        count++;
      }
    }

    // Copy to second output
    svtkPoints* points2 = input->GetPoints()->NewInstance();
    points2->SetDataType(input->GetPoints()->GetDataType());
    points2->SetNumberOfPoints(count - 1);
    output2->SetPoints(points2);

    MapOutliers outliersWorker;
    inPtArray = input->GetPoints()->GetData();
    outPtArray = output2->GetPoints()->GetData();
    // Fast path for float/double:
    if (!Dispatcher::Execute(inPtArray, outPtArray, outliersWorker, this->PointMap, inPD, outPD2))
    { // fallback for weird types:
      outliersWorker(inPtArray, outPtArray, this->PointMap, inPD, outPD2);
    }
    points2->Delete();

    // Produce poly vertex cell if requested
    this->GenerateVerticesIfRequested(output2);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkPointCloudFilter::GenerateVerticesIfRequested(svtkPolyData* output)
{
  svtkIdType numPts;
  if (!this->GenerateVertices || output->GetPoints() == nullptr ||
    (numPts = output->GetNumberOfPoints()) < 1)
  {
    return;
  }

  // Okay create a cell array and assign it to the output
  svtkCellArray* verts = svtkCellArray::New();
  verts->AllocateEstimate(1, numPts);

  verts->InsertNextCell(numPts);
  for (svtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    verts->InsertCellPoint(ptId);
  }

  output->SetVerts(verts);
  verts->Delete();
}

//----------------------------------------------------------------------------
int svtkPointCloudFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPointCloudFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Points Removed: " << this->NumberOfPointsRemoved << "\n";

  os << indent << "Generate Outliers: " << (this->GenerateOutliers ? "On\n" : "Off\n");

  os << indent << "Generate Vertices: " << (this->GenerateVertices ? "On\n" : "Off\n");
}
