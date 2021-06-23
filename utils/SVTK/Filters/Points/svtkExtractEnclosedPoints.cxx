/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractEnclosedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractEnclosedPoints.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkFeatureEdges.h"
#include "svtkGarbageCollector.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntersectionCounter.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRandomPool.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSelectEnclosedPoints.h"
#include "svtkStaticCellLocator.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>

svtkStandardNewMacro(svtkExtractEnclosedPoints);

//----------------------------------------------------------------------------
// Classes support threading. Each point can be processed separately, so the
// in/out containment check is threaded.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Thread on point type.
template <typename ArrayT>
struct ExtractInOutCheck
{
  ArrayT* Points;
  svtkPolyData* Surface;
  double Bounds[6];
  double Length;
  double Tolerance;
  svtkStaticCellLocator* Locator;
  svtkIdType* PointMap;
  svtkRandomPool* Sequence;
  svtkSMPThreadLocal<svtkIntersectionCounter> Counter;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage eliminates lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> CellIds;
  svtkSMPThreadLocalObject<svtkGenericCell> Cell;

  ExtractInOutCheck(ArrayT* pts, svtkPolyData* surface, double bds[6], double tol,
    svtkStaticCellLocator* loc, svtkIdType* map)
    : Points(pts)
    , Surface(surface)
    , Tolerance(tol)
    , Locator(loc)
    , PointMap(map)
  {
    const svtkIdType numPts = pts->GetNumberOfTuples();

    this->Bounds[0] = bds[0];
    this->Bounds[1] = bds[1];
    this->Bounds[2] = bds[2];
    this->Bounds[3] = bds[3];
    this->Bounds[4] = bds[4];
    this->Bounds[5] = bds[5];
    this->Length = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));

    // Precompute a sufficiently large enough random sequence
    this->Sequence = svtkRandomPool::New();
    this->Sequence->SetSize(std::max(numPts, svtkIdType{ 1500 }));
    this->Sequence->GeneratePool();
  }

  ~ExtractInOutCheck() { this->Sequence->Delete(); }

  void Initialize()
  {
    svtkIdList*& cellIds = this->CellIds.Local();
    cellIds->Allocate(512);
    svtkIntersectionCounter& counter = this->Counter.Local();
    counter.SetTolerance(this->Tolerance);
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3];
    const auto points = svtk::DataArrayTupleRange(this->Points);
    svtkIdType* map = this->PointMap + ptId;
    svtkGenericCell*& cell = this->Cell.Local();
    svtkIdList*& cellIds = this->CellIds.Local();
    svtkIdType hit;
    svtkIntersectionCounter& counter = this->Counter.Local();

    for (; ptId < endPtId; ++ptId)
    {
      const auto pt = points[ptId];

      x[0] = static_cast<double>(pt[0]);
      x[1] = static_cast<double>(pt[1]);
      x[2] = static_cast<double>(pt[2]);

      hit = svtkSelectEnclosedPoints::IsInsideSurface(x, this->Surface, this->Bounds, this->Length,
        this->Tolerance, this->Locator, cellIds, cell, counter, this->Sequence, ptId);
      *map++ = (hit ? 1 : -1);
    }
  }

  void Reduce() {}
}; // ExtractInOutCheck

struct ExtractLauncher
{
  template <typename ArrayT>
  void operator()(ArrayT* pts, svtkPolyData* surface, double bds[6], double tol,
    svtkStaticCellLocator* loc, svtkIdType* hits)
  {
    ExtractInOutCheck<ArrayT> inOut(pts, surface, bds, tol, loc, hits);
    svtkSMPTools::For(0, pts->GetNumberOfTuples(), inOut);
  }
};

} // anonymous namespace

//----------------------------------------------------------------------------
// Construct object.
svtkExtractEnclosedPoints::svtkExtractEnclosedPoints()
{
  this->SetNumberOfInputPorts(2);

  this->CheckSurface = false;
  this->Tolerance = 0.001;
}

//----------------------------------------------------------------------------
svtkExtractEnclosedPoints::~svtkExtractEnclosedPoints() = default;

//----------------------------------------------------------------------------
// Partial implementation invokes svtkPointCloudFilter::RequestData(). This is
// necessary to grab the seconf input.
//
int svtkExtractEnclosedPoints::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);

  // get the second input
  svtkPolyData* surface = svtkPolyData::SafeDownCast(in2Info->Get(svtkDataObject::DATA_OBJECT()));
  this->Surface = surface;

  svtkDebugMacro("Extracting enclosed points");

  // If requested, check that the surface is closed
  if (this->Surface == nullptr ||
    (this->CheckSurface && !svtkSelectEnclosedPoints::IsSurfaceClosed(surface)))
  {
    svtkErrorMacro("Bad enclosing surface");
    return 0;
  }

  // Okay take advantage of superclasses' RequestData() method. This provides
  // This provides a lot of the point mapping, attribute copying, etc.
  // capabilities.
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// the enclosing surface.
int svtkExtractEnclosedPoints::FilterPoints(svtkPointSet* input)
{
  // Initiailize search structures
  svtkStaticCellLocator* locator = svtkStaticCellLocator::New();

  svtkPolyData* surface = this->Surface;
  double bds[6];
  surface->GetBounds(bds);

  // Set up structures for acceleration ray casting
  locator->SetDataSet(surface);
  locator->BuildLocator();

  // Loop over all input points determining inside/outside
  // Use fast path for float/double points:
  using svtkArrayDispatch::Reals;
  using Dispatcher = svtkArrayDispatch::DispatchByValueType<Reals>;
  ExtractLauncher worker;
  svtkDataArray* ptArray = input->GetPoints()->GetData();
  if (!Dispatcher::Execute(ptArray, worker, surface, bds, this->Tolerance, locator, this->PointMap))
  { // fallback for other arrays:
    worker(ptArray, surface, bds, this->Tolerance, locator, this->PointMap);
  }

  // Clean up and get out
  locator->Delete();
  return 1;
}

//----------------------------------------------------------------------------
// Specify the second enclosing surface input via a connection
void svtkExtractEnclosedPoints::SetSurfaceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify the second enclosing surface input data
void svtkExtractEnclosedPoints::SetSurfaceData(svtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Return the enclosing surface
svtkPolyData* svtkExtractEnclosedPoints::GetSurface()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkExtractEnclosedPoints::GetSurface(svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int svtkExtractEnclosedPoints::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractEnclosedPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Check Surface: " << (this->CheckSurface ? "On\n" : "Off\n");

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
