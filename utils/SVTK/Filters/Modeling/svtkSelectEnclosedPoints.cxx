/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectEnclosedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSelectEnclosedPoints.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkFeatureEdges.h"
#include "svtkGarbageCollector.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRandomPool.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticCellLocator.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkSelectEnclosedPoints);

//----------------------------------------------------------------------------
// Classes support threading. Each point can be processed separately, so the
// in/out containment check is threaded.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Thread on point type.
struct SelectInOutCheck
{
  svtkIdType NumPts;
  svtkDataSet* DataSet;
  svtkPolyData* Surface;
  double Bounds[6];
  double Length;
  double Tolerance;
  svtkStaticCellLocator* Locator;
  unsigned char* Hits;
  svtkSelectEnclosedPoints* Selector;
  svtkTypeBool InsideOut;
  svtkRandomPool* Sequence;
  svtkSMPThreadLocal<svtkIntersectionCounter> Counter;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage eliminates lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> CellIds;
  svtkSMPThreadLocalObject<svtkGenericCell> Cell;

  SelectInOutCheck(svtkIdType numPts, svtkDataSet* ds, svtkPolyData* surface, double bds[6],
    double tol, svtkStaticCellLocator* loc, unsigned char* hits, svtkSelectEnclosedPoints* sel,
    svtkTypeBool io)
    : NumPts(numPts)
    , DataSet(ds)
    , Surface(surface)
    , Tolerance(tol)
    , Locator(loc)
    , Hits(hits)
    , Selector(sel)
    , InsideOut(io)
  {
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
    this->Sequence->SetSize((numPts > 1500 ? numPts : 1500));
    this->Sequence->GeneratePool();
  }

  ~SelectInOutCheck() { this->Sequence->Delete(); }

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
    unsigned char* hits = this->Hits + ptId;
    svtkGenericCell*& cell = this->Cell.Local();
    svtkIdList*& cellIds = this->CellIds.Local();
    svtkIntersectionCounter& counter = this->Counter.Local();

    for (; ptId < endPtId; ++ptId)
    {
      this->DataSet->GetPoint(ptId, x);

      if (this->Selector->IsInsideSurface(x, this->Surface, this->Bounds, this->Length,
            this->Tolerance, this->Locator, cellIds, cell, counter, this->Sequence, ptId))
      {
        *hits++ = (this->InsideOut ? 0 : 1);
      }
      else
      {
        *hits++ = (this->InsideOut ? 1 : 0);
      }
    }
  }

  void Reduce() {}

  static void Execute(svtkIdType numPts, svtkDataSet* ds, svtkPolyData* surface, double bds[6],
    double tol, svtkStaticCellLocator* loc, unsigned char* hits, svtkSelectEnclosedPoints* sel)
  {
    SelectInOutCheck inOut(numPts, ds, surface, bds, tol, loc, hits, sel, sel->GetInsideOut());
    svtkSMPTools::For(0, numPts, inOut);
  }
}; // SelectInOutCheck

} // anonymous namespace

//----------------------------------------------------------------------------
// Construct object.
svtkSelectEnclosedPoints::svtkSelectEnclosedPoints()
{
  this->SetNumberOfInputPorts(2);

  this->CheckSurface = false;
  this->InsideOut = 0;
  this->Tolerance = 0.0001;

  this->InsideOutsideArray = nullptr;

  // These are needed to support backward compatibility
  this->CellLocator = svtkStaticCellLocator::New();
  this->CellIds = svtkIdList::New();
  this->Cell = svtkGenericCell::New();
}

//----------------------------------------------------------------------------
svtkSelectEnclosedPoints::~svtkSelectEnclosedPoints()
{
  if (this->InsideOutsideArray)
  {
    this->InsideOutsideArray->Delete();
  }

  if (this->CellLocator)
  {
    svtkAbstractCellLocator* loc = this->CellLocator;
    this->CellLocator = nullptr;
    loc->Delete();
  }

  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
int svtkSelectEnclosedPoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the two inputs and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* surface = svtkPolyData::SafeDownCast(in2Info->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro("Selecting enclosed points");

  // If requested, check that the surface is closed
  if (this->CheckSurface && !this->IsSurfaceClosed(surface))
  {
    return 0;
  }

  // Initiailize search structures
  this->Initialize(surface);

  // Create array to mark inside/outside
  if (this->InsideOutsideArray)
  {
    this->InsideOutsideArray->Delete();
  }
  this->InsideOutsideArray = svtkUnsignedCharArray::New();
  svtkUnsignedCharArray* hits = this->InsideOutsideArray;

  // Loop over all input points determining inside/outside
  svtkIdType numPts = input->GetNumberOfPoints();
  hits->SetNumberOfValues(numPts);
  unsigned char* hitsPtr = static_cast<unsigned char*>(hits->GetVoidPointer(0));

  // Process the points in parallel
  SelectInOutCheck::Execute(
    numPts, input, surface, this->Bounds, this->Tolerance, this->CellLocator, hitsPtr, this);

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  hits->SetName("SelectedPoints");
  output->GetPointData()->SetScalars(hits);

  // release memory
  this->Complete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkSelectEnclosedPoints::IsSurfaceClosed(svtkPolyData* surface)
{
  svtkPolyData* checker = svtkPolyData::New();
  checker->CopyStructure(surface);

  svtkFeatureEdges* features = svtkFeatureEdges::New();
  features->SetInputData(checker);
  features->BoundaryEdgesOn();
  features->NonManifoldEdgesOn();
  features->ManifoldEdgesOff();
  features->FeatureEdgesOff();
  features->Update();

  svtkIdType numCells = features->GetOutput()->GetNumberOfCells();
  features->Delete();
  checker->Delete();

  if (numCells > 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
void svtkSelectEnclosedPoints::Initialize(svtkPolyData* surface)
{
  if (!this->CellLocator)
  {
    this->CellLocator = svtkStaticCellLocator::New();
  }

  this->Surface = surface;
  surface->GetBounds(this->Bounds);
  this->Length = surface->GetLength();

  // Set up structures for acceleration ray casting
  this->CellLocator->SetDataSet(surface);
  this->CellLocator->BuildLocator();
}

//----------------------------------------------------------------------------
int svtkSelectEnclosedPoints::IsInside(svtkIdType inputPtId)
{
  if (!this->InsideOutsideArray || this->InsideOutsideArray->GetValue(inputPtId) == 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
int svtkSelectEnclosedPoints::IsInsideSurface(double x, double y, double z)
{
  double xyz[3];
  xyz[0] = x;
  xyz[1] = y;
  xyz[2] = z;
  return this->IsInsideSurface(xyz);
}

//----------------------------------------------------------------------------
// This is done to preserve backward compatibility. However it is not thread
// safe due to the use of the data member CellIds and Cell.
int svtkSelectEnclosedPoints::IsInsideSurface(double x[3])
{
  svtkIntersectionCounter counter(this->Tolerance, this->Length);

  return this->IsInsideSurface(x, this->Surface, this->Bounds, this->Length, this->Tolerance,
    this->CellLocator, this->CellIds, this->Cell, counter);
}

//----------------------------------------------------------------------------
// General method uses ray casting to determine in/out. Since this is a
// numerically delicate operation, we use a crude "statistical" method (based
// on voting) to provide a better answer. Plus there is a process to merge
// nearly conincident points along the intersection rays.
//
// This is a static method so it can be used by other filters; hence the
// many parameters used.
//
// Provision for reproducible threaded random number generation is made by
// supporting the precomputation of a random sequence (see svtkRandomPool).
//
#define SVTK_MAX_ITER 10      // Maximum iterations for ray-firing
#define SVTK_VOTE_THRESHOLD 2 // Vote margin for test

int svtkSelectEnclosedPoints::IsInsideSurface(double x[3], svtkPolyData* surface, double bds[6],
  double length, double tolerance, svtkAbstractCellLocator* locator, svtkIdList* cellIds,
  svtkGenericCell* genCell, svtkIntersectionCounter& counter, svtkRandomPool* seq, svtkIdType seqIdx)
{
  // do a quick inside bounds check against the surface bounds
  if (x[0] < bds[0] || x[0] > bds[1] || x[1] < bds[2] || x[1] > bds[3] || x[2] < bds[4] ||
    x[2] > bds[5])
  {
    return 0;
  }

  // Shortly we are going to start firing rays. It's important that the rays
  // are long enough to go from the test point all the way through the
  // enclosing surface. So compute a vector from the test point to the center
  // of the surface, and then add in the length (diagonal of bounding box) of
  // the surface.
  double offset[3], totalLength;
  offset[0] = x[0] - ((bds[0] + bds[1]) / 2.0);
  offset[1] = x[1] - ((bds[2] + bds[3]) / 2.0);
  offset[2] = x[2] - ((bds[4] + bds[5]) / 2.0);
  totalLength = length + svtkMath::Norm(offset);

  //  Perform in/out by shooting random rays. Multiple rays are fired
  //  to improve accuracy of the result.
  //
  //  The variable iterNumber counts the number of rays fired and is
  //  limited by the defined variable SVTK_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the surface.  When deltaVotes > 0, more votes
  //  have counted for "in" than "out".  When deltaVotes < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable SVTK_VOTE_THRESHOLD, then the
  //  appropriate "in" or "out" status is returned.
  //
  double rayMag, ray[3], xray[3], t, pcoords[3], xint[3];
  int i, numInts, iterNumber, deltaVotes, subId;
  svtkIdType idx, numCells;
  double tol = tolerance * length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < SVTK_MAX_ITER) && (abs(deltaVotes) < SVTK_VOTE_THRESHOLD); iterNumber++)
  {
    //  Define a random ray to fire.
    rayMag = 0.0;
    while (rayMag == 0.0)
    {
      if (seq == nullptr) // in serial mode
      {
        ray[0] = svtkMath::Random(-1.0, 1.0);
        ray[1] = svtkMath::Random(-1.0, 1.0);
        ray[2] = svtkMath::Random(-1.0, 1.0);
      }
      else // threading, have to scale sequence -1<=x<=1
      {
        ray[0] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
        ray[1] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
        ray[2] = 2.0 * (0.5 - seq->GetValue(seqIdx++));
      }
      rayMag = svtkMath::Norm(ray);
    }

    // The ray must be appropriately sized wrt the bounding box. (It has to
    // go all the way through the bounding box. Remember though that an
    // "inside bounds" check was done previously so diagonal length should
    // be long enough.)
    for (i = 0; i < 3; i++)
    {
      xray[i] = x[i] + 2.0 * totalLength * (ray[i] / rayMag);
    }

    // Retrieve the candidate cells from the locator to limit the
    // intersections to be attempted.
    locator->FindCellsAlongLine(x, xray, tol, cellIds);
    numCells = cellIds->GetNumberOfIds();

    counter.Reset();
    for (idx = 0; idx < numCells; idx++)
    {
      surface->GetCell(cellIds->GetId(idx), genCell);
      if (genCell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId))
      {
        counter.AddIntersection(t);
      }
    } // for all candidate cells along this ray

    numInts = counter.CountIntersections();

    if ((numInts % 2) == 0) // if outside
    {
      --deltaVotes;
    }
    else // if inside
    {
      ++deltaVotes;
    }
  } // try another ray

  //   If the number of votes is positive, the point is inside
  //
  return (deltaVotes < 0 ? 0 : 1);
}

#undef SVTK_MAX_ITER
#undef SVTK_VOTE_THRESHOLD

//----------------------------------------------------------------------------
// Specify the second enclosing surface input via a connection
void svtkSelectEnclosedPoints::SetSurfaceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify the second enclosing surface input data
void svtkSelectEnclosedPoints::SetSurfaceData(svtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Return the enclosing surface
svtkPolyData* svtkSelectEnclosedPoints::GetSurface()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkSelectEnclosedPoints::GetSurface(svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int svtkSelectEnclosedPoints::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
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
void svtkSelectEnclosedPoints::Complete()
{
  this->CellLocator->FreeSearchStructure();
}

//----------------------------------------------------------------------------
void svtkSelectEnclosedPoints::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  svtkGarbageCollectorReport(collector, this->CellLocator, "CellLocator");
}

//----------------------------------------------------------------------------
void svtkSelectEnclosedPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Check Surface: " << (this->CheckSurface ? "On\n" : "Off\n");

  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
