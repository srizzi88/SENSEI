/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDensifyPointCloudFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDensifyPointCloudFilter.h"

#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkFloatArray.h"
#include "svtkIdList.h"
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

svtkStandardNewMacro(svtkDensifyPointCloudFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// Count the number of points that need generation
template <typename T>
struct CountPoints
{
  T* InPoints;
  svtkStaticPointLocator* Locator;
  svtkIdType* Count;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> PIds;

  CountPoints(T* inPts, svtkStaticPointLocator* loc, svtkIdType* count, int ntype, int nclose,
    double r, double d)
    : InPoints(inPts)
    , Locator(loc)
    , Count(count)
    , NeighborhoodType(ntype)
    , NClosest(nclose)
    , Radius(r)
    , Distance(d)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType pointId, svtkIdType endPointId)
  {
    T *x, *p = this->InPoints + 3 * pointId;
    svtkStaticPointLocator* loc = this->Locator;
    svtkIdType* count = this->Count + pointId;
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType i, id, numIds, numNewPts;
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for (; pointId < endPointId; ++pointId, p += 3)
    {
      numNewPts = 0;
      px[0] = static_cast<double>(p[0]);
      px[1] = static_cast<double>(p[1]);
      px[2] = static_cast<double>(p[2]);
      if (ntype == svtkDensifyPointCloudFilter::N_CLOSEST)
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose + 1, px, pIds);
      }
      else // ntype == svtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for (i = 0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if (id > pointId) // only process points of larger id
        {
          x = this->InPoints + 3 * id;
          py[0] = static_cast<double>(x[0]);
          py[1] = static_cast<double>(x[1]);
          py[2] = static_cast<double>(x[2]);

          if (svtkMath::Distance2BetweenPoints(px, py) >= d2)
          {
            numNewPts++;
          }
        } // larger id
      }   // for all neighbors
      *count++ = numNewPts;
    } // for all points in this batch
  }

  void Reduce() {}

  static void Execute(svtkIdType numPts, T* pts, svtkStaticPointLocator* loc, svtkIdType* count,
    int ntype, int nclose, double r, double d)
  {
    CountPoints countPts(pts, loc, count, ntype, nclose, r, d);
    svtkSMPTools::For(0, numPts, countPts);
  }

}; // CountPoints

//----------------------------------------------------------------------------
// Count the number of points that need generation
template <typename T>
struct GeneratePoints
{
  T* InPoints;
  svtkStaticPointLocator* Locator;
  const svtkIdType* Offsets;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;
  ArrayList Arrays;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> PIds;

  GeneratePoints(T* inPts, svtkStaticPointLocator* loc, svtkIdType* offset, int ntype, int nclose,
    double r, double d, svtkIdType numPts, svtkPointData* attr)
    : InPoints(inPts)
    , Locator(loc)
    , Offsets(offset)
    , NeighborhoodType(ntype)
    , NClosest(nclose)
    , Radius(r)
    , Distance(d)
  {
    this->Arrays.AddSelfInterpolatingArrays(numPts, attr);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType pointId, svtkIdType endPointId)
  {
    T *x, *p = this->InPoints + 3 * pointId;
    T* newX;
    svtkStaticPointLocator* loc = this->Locator;
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType i, id, numIds;
    svtkIdType outPtId = this->Offsets[pointId];
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for (; pointId < endPointId; ++pointId, p += 3)
    {
      px[0] = static_cast<double>(p[0]);
      px[1] = static_cast<double>(p[1]);
      px[2] = static_cast<double>(p[2]);
      if (ntype == svtkDensifyPointCloudFilter::N_CLOSEST)
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose + 1, px, pIds);
      }
      else // ntype == svtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for (i = 0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if (id > pointId) // only process points of larger id
        {
          x = this->InPoints + 3 * id;
          py[0] = static_cast<double>(x[0]);
          py[1] = static_cast<double>(x[1]);
          py[2] = static_cast<double>(x[2]);

          if (svtkMath::Distance2BetweenPoints(px, py) >= d2)
          {
            newX = this->InPoints + 3 * outPtId;
            *newX++ = static_cast<T>(0.5 * (px[0] + py[0]));
            *newX++ = static_cast<T>(0.5 * (px[1] + py[1]));
            *newX++ = static_cast<T>(0.5 * (px[2] + py[2]));
            this->Arrays.InterpolateEdge(pointId, id, 0.5, outPtId);
            outPtId++;
          }
        } // larger id
      }   // for all neighbor points
    }     // for all points in this batch
  }

  void Reduce() {}

  static void Execute(svtkIdType numInPts, T* pts, svtkStaticPointLocator* loc, svtkIdType* offsets,
    int ntype, int nclose, double r, double d, svtkIdType numOutPts, svtkPointData* PD)
  {
    GeneratePoints genPts(pts, loc, offsets, ntype, nclose, r, d, numOutPts, PD);
    svtkSMPTools::For(0, numInPts, genPts);
  }

}; // GeneratePoints

} // anonymous namespace

//================= Begin SVTK class proper =======================================
//----------------------------------------------------------------------------
svtkDensifyPointCloudFilter::svtkDensifyPointCloudFilter()
{

  this->NeighborhoodType = svtkDensifyPointCloudFilter::N_CLOSEST;
  this->Radius = 1.0;
  this->NumberOfClosestPoints = 6;
  this->TargetDistance = 0.5;
  this->MaximumNumberOfIterations = 3;
  this->InterpolateAttributeData = true;
  this->MaximumNumberOfPoints = SVTK_ID_MAX;
}

//----------------------------------------------------------------------------
svtkDensifyPointCloudFilter::~svtkDensifyPointCloudFilter() = default;

//----------------------------------------------------------------------------
// Produce the output data
int svtkDensifyPointCloudFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Start by building the locator, creating the output points and otherwise
  // and prepare for iteration.
  int iterNum;
  svtkStaticPointLocator* locator = svtkStaticPointLocator::New();

  svtkPoints* inPts = input->GetPoints();
  int pointsType = inPts->GetDataType();
  svtkPoints* newPts = inPts->NewInstance();
  newPts->DeepCopy(inPts);
  output->SetPoints(newPts);
  svtkPointData* outPD = nullptr;
  if (this->InterpolateAttributeData)
  {
    outPD = output->GetPointData();
    outPD->DeepCopy(input->GetPointData());
    outPD->InterpolateAllocate(outPD, numPts);
  }

  svtkIdType ptId, numInPts, numNewPts;
  svtkIdType npts, offset, *offsets;
  void* pts = nullptr;
  double d = this->TargetDistance;

  // Loop over the data, bisecting connecting edges as required.
  for (iterNum = 0; iterNum < this->MaximumNumberOfIterations; ++iterNum)
  {
    // Prepare to process
    locator->SetDataSet(output);
    locator->Modified();
    locator->BuildLocator();

    // Count the number of points to create
    numInPts = output->GetNumberOfPoints();
    offsets = new svtkIdType[numInPts];
    pts = output->GetPoints()->GetVoidPointer(0);
    switch (pointsType)
    {
      svtkTemplateMacro(CountPoints<SVTK_TT>::Execute(numInPts, (SVTK_TT*)pts, locator, offsets,
        this->NeighborhoodType, this->NumberOfClosestPoints, this->Radius, d));
    }

    // Prefix sum to count the number of points created and build offsets
    numNewPts = 0;
    offset = numInPts;
    for (ptId = 0; ptId < numInPts; ++ptId)
    {
      npts = offsets[ptId];
      offsets[ptId] = offset;
      offset += npts;
    }
    numNewPts = offset - numInPts;

    // Check convergence
    if (numNewPts == 0 || offset > this->MaximumNumberOfPoints)
    {
      delete[] offsets;
      break;
    }

    // Now add points and attribute data if requested. Allocate memory
    // for points and attributes.
    newPts->InsertPoint(offset, 0.0, 0.0, 0.0); // side effect reallocs memory
    pts = output->GetPoints()->GetVoidPointer(0);
    switch (pointsType)
    {
      svtkTemplateMacro(GeneratePoints<SVTK_TT>::Execute(numInPts, (SVTK_TT*)pts, locator, offsets,
        this->NeighborhoodType, this->NumberOfClosestPoints, this->Radius, d, offset, outPD));
    }

    delete[] offsets;
  } // while max num of iterations not exceeded

  // Clean up
  locator->Delete();
  newPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkDensifyPointCloudFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkDensifyPointCloudFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Neighborhood Type: " << this->GetNeighborhoodType() << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number Of Closest Points: " << this->NumberOfClosestPoints << "\n";
  os << indent << "Target Distance: " << this->TargetDistance << endl;
  os << indent << "Maximum Number of Iterations: " << this->MaximumNumberOfIterations << "\n";
  os << indent
     << "Interpolate Attribute Data: " << (this->InterpolateAttributeData ? "On\n" : "Off\n");
  os << indent << "Maximum Number Of Points: " << this->MaximumNumberOfPoints << "\n";
}
