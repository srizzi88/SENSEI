/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpanSpace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSpanSpace.h"

#include "svtkCell.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkUnstructuredGrid.h"

// Methods and functors for processing in parallel
namespace
{ // begin anonymous namespace

// Compute the scalar range a little faster
template <typename T>
struct ComputeRange
{
  struct LocalDataType
  {
    double Min;
    double Max;
  };

  const T* Scalars;
  double Min;
  double Max;
  svtkSMPThreadLocal<LocalDataType> LocalData;

  ComputeRange(T* s)
    : Scalars(s)
    , Min(SVTK_FLOAT_MAX)
    , Max(SVTK_FLOAT_MIN)
  {
  }

  void Initialize()
  {
    LocalDataType& localData = this->LocalData.Local();
    localData.Min = SVTK_FLOAT_MAX;
    localData.Max = SVTK_FLOAT_MIN;
  }

  void operator()(svtkIdType idx, svtkIdType endIdx)
  {
    LocalDataType& localData = this->LocalData.Local();
    double& min = localData.Min;
    double& max = localData.Max;
    const T* s = this->Scalars + idx;

    for (; idx < endIdx; ++idx, ++s)
    {
      min = (*s < min ? *s : min);
      max = (*s > max ? *s : max);
    }
  }

  void Reduce()
  {
    typename svtkSMPThreadLocal<LocalDataType>::iterator ldItr;
    typename svtkSMPThreadLocal<LocalDataType>::iterator ldEnd = this->LocalData.end();

    this->Min = SVTK_FLOAT_MAX;
    this->Max = SVTK_FLOAT_MIN;

    double min, max;
    for (ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      min = (*ldItr).Min;
      max = (*ldItr).Max;
      this->Min = (min < this->Min ? min : this->Min);
      this->Max = (max > this->Max ? max : this->Max);
    }
  }

  static void Execute(svtkIdType num, T* s, double range[2])
  {
    ComputeRange computeRange(s);
    svtkSMPTools::For(0, num, computeRange);
    range[0] = computeRange.Min;
    range[1] = computeRange.Max;
  }
};

//-----------------------------------------------------------------------------
// The following tuple is an interface between SVTK class and internal class
struct svtkSpanTuple
{
  svtkIdType CellId; // originating cellId
  svtkIdType Index;  // i-j index into span space (numCells in length)
  // Operator< used to support sorting operation. Note that the sorting
  // occurs over both the index and cell id. This arranges cells in
  // ascending order (within a bin) which often makes a difference
  //(~10-15%) in large data as it reduces cache misses.
  bool operator<(const svtkSpanTuple& tuple) const
  {
    if (Index < tuple.Index)
      return true;
    if (tuple.Index < Index)
      return false;
    if (CellId < tuple.CellId)
      return true;
    return false;
  }
};

} // anonymous

//-----------------------------------------------------------------------------
// This class manages the span space, including methods to create, access, and
// delete it.
struct svtkInternalSpanSpace
{
  // Okay the various ivars
  svtkIdType Dim;             // the number of rows and number of columns
  double SMin, SMax, Range;  // min and max scalar values; range
  svtkSpanTuple* Space;       //(cellId,s) span space tuples
  svtkIdType* CellIds;        // sorted list of cell ids
  svtkIdType* Offsets;        // offset into CellIds for each bucket (Dim*Dim in size)
  svtkIdType NumCells;        // total number of cells in span space
  svtkIdType* CandidateCells; // to support parallel computing
  svtkIdType NumCandidates;

  // Constructor
  svtkInternalSpanSpace(svtkIdType dim, double sMin, double sMax, svtkIdType numCells);

  // Destructore
  ~svtkInternalSpanSpace();

  // Insert cells with scalar range (smin,smax) in span space. These are
  // sorted later into span space.
  void SetSpanPoint(svtkIdType id, double sMin, double sMax)
  {
    svtkIdType i =
      static_cast<svtkIdType>(static_cast<double>(this->Dim) * (sMin - this->SMin) / this->Range);
    svtkIdType j =
      static_cast<svtkIdType>(static_cast<double>(this->Dim) * (sMax - this->SMin) / this->Range);
    i = (i < 0 ? 0 : (i >= this->Dim ? this->Dim - 1 : i));
    j = (j < 0 ? 0 : (j >= this->Dim ? this->Dim - 1 : j));
    this->Space[id].CellId = id;
    this->Space[id].Index = i + j * Dim;
  }

  // Do the hard work of sorting and arranging the span space
  void Build();

  // Given a scalar value, return a rectangle in span space. This
  // rectangle is used subsequently for extracting individual
  // rows. rMin is the lower (i,j) lower-left corner of the rectangle;
  // rMax is the upper-right corner (i,j) position of the
  // rectangle.
  void GetSpanRectangle(double value, svtkIdType rMin[2], svtkIdType rMax[2])
  {
    svtkIdType i =
      static_cast<svtkIdType>(static_cast<double>(this->Dim) * (value - this->SMin) / this->Range);

    // In the case where value is outside of the span tree scalar range, need
    // to return an empty span rectangle.
    if (i < 0 || i >= this->Dim)
    {
      rMin[0] = rMin[1] = rMax[0] = rMax[1] = 0;
    }
    else // return a non-empty span rectangle
    {
      rMin[0] = 0;     // xmin on rectangle left boundary
      rMin[1] = i;     // ymin on rectangle bottom
      rMax[0] = i + 1; // xmax (non-inclusive interval) on right hand boundary
      rMax[1] = Dim;   // ymax (non-inclusive interval) on top boundary of span space
    }
  }

  // Return an array of cellIds along a prescribed row within the span
  // rectangle.  Note that the row should be inside the
  // rectangle. Note that numCells may be zero in which case the
  // pointer returned will not point to valid data.
  svtkIdType* GetCellsInSpan(
    svtkIdType row, svtkIdType rMin[2], svtkIdType rMax[2], svtkIdType& numCells)
  {
    // Find the beginning of some cells on this row.
    svtkIdType startOffset = *(this->Offsets + row * this->Dim + rMin[0]);
    svtkIdType endOffset = *(this->Offsets + row * this->Dim + rMax[0]);
    numCells = endOffset - startOffset;
    return this->CellIds + startOffset;
  }
};

//-----------------------------------------------------------------------------
svtkInternalSpanSpace::svtkInternalSpanSpace(
  svtkIdType dim, double sMin, double sMax, svtkIdType numCells)
{
  this->Dim = dim;
  this->SMin = sMin;
  this->SMax = sMax;
  this->Range = (sMax - sMin);
  this->Offsets = new svtkIdType[dim * dim + 1]; // leave one extra for numCells
  std::fill_n(this->Offsets, dim * dim, 0);
  this->NumCells = numCells;
  this->Space = new svtkSpanTuple[numCells];
  this->CellIds = new svtkIdType[numCells];
  this->CandidateCells = nullptr;
  this->NumCandidates = 0;
}

//-----------------------------------------------------------------------------
svtkInternalSpanSpace::~svtkInternalSpanSpace()
{
  delete[] this->Offsets;
  delete[] this->Space;
  delete[] this->CellIds;
  delete[] this->CandidateCells;
}

//-----------------------------------------------------------------------------
// The heart of the algorithm. The cells are sorted in i-j space into
// a contiguous array. Then the offsets into the array are built.
void svtkInternalSpanSpace::Build()
{
  // The first thing to do is to sort the elements across span
  // space. The shape of the span space is upper diagonal (because
  // smax >= smin) but for simplicity sake (for now) we just use a
  // rectangular discretization (of dimensions Dim*Dim).
  svtkSMPTools::Sort(this->Space, this->Space + this->NumCells);

  // Now that this is done, we create a matrix of offsets into the
  // sorted array. This enables rapid access into the sorted cellIds,
  // including access to span space rows of cells.  Also for
  // convenience we replicate the cell ids. This further supports
  // parallel traversal which is a common use case. If I was smarter I
  // could use the CellIds already contained in the tuple and not have
  // to duplicate this, but then sorting requires a custom class with
  // iterators, etc.

  // First count the number of contributions in each bucket.
  svtkIdType cellId, numElems;
  for (cellId = 0; cellId < this->NumCells; ++cellId)
  {
    this->Offsets[this->Space[cellId].Index]++;
    this->CellIds[cellId] = this->Space[cellId].CellId;
  }

  // Now accumulate offset array
  svtkIdType i, j, jOffset, idx, currentOffset = 0;
  for (j = 0; j < this->Dim; ++j)
  {
    jOffset = j * this->Dim;
    for (i = 0; i < this->Dim; ++i)
    {
      idx = i + jOffset;
      numElems = this->Offsets[idx];
      this->Offsets[idx] = currentOffset;
      currentOffset += numElems;
    }
  }
  this->Offsets[this->Dim * this->Dim] = this->NumCells;

  // We don't need the span space tuple array any more, we have
  // offsets and cell ids computed.
  delete[] this->Space;
  this->Space = nullptr;
}

namespace
{ // begin anonymous namespace

// Generic method to map cells to span space. Uses GetCellPoints() to retrieve
// points defining each cell.
struct MapToSpanSpace
{
  svtkInternalSpanSpace* SpanSpace;
  svtkDataSet* DataSet;
  svtkDataArray* Scalars;
  svtkSMPThreadLocalObject<svtkIdList> CellPts;
  svtkSMPThreadLocalObject<svtkDoubleArray> CellScalars;

  MapToSpanSpace(svtkInternalSpanSpace* ss, svtkDataSet* ds, svtkDataArray* s)
    : SpanSpace(ss)
    , DataSet(ds)
    , Scalars(s)
  {
  }

  void Initialize()
  {
    svtkIdList*& cellPts = this->CellPts.Local();
    cellPts->SetNumberOfIds(12);
    svtkDoubleArray*& cellScalars = this->CellScalars.Local();
    cellScalars->SetNumberOfTuples(12);
  }

  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    svtkIdType j, numScalars;
    double *s, sMin, sMax;
    svtkIdList*& cellPts = this->CellPts.Local();
    svtkDoubleArray*& cellScalars = this->CellScalars.Local();

    for (; cellId < endCellId; ++cellId)
    {
      this->DataSet->GetCellPoints(cellId, cellPts);
      numScalars = cellPts->GetNumberOfIds();
      cellScalars->SetNumberOfTuples(numScalars);
      this->Scalars->GetTuples(cellPts, cellScalars);
      s = cellScalars->GetPointer(0);

      sMin = SVTK_DOUBLE_MAX;
      sMax = SVTK_DOUBLE_MIN;
      for (j = 0; j < numScalars; j++)
      {
        if (s[j] < sMin)
        {
          sMin = s[j];
        }
        if (s[j] > sMax)
        {
          sMax = s[j];
        }
      } // for all cell scalars
      // Compute span space id, and prepare to map
      this->SpanSpace->SetSpanPoint(cellId, sMin, sMax);
    } // for all cells in this thread
  }

  void Reduce() // Needed because of Initialize()
  {
  }

  static void Execute(svtkIdType numCells, svtkInternalSpanSpace* ss, svtkDataSet* ds, svtkDataArray* s)
  {
    MapToSpanSpace map(ss, ds, s);
    svtkSMPTools::For(0, numCells, map);
  }
}; // MapToSpanSpace

// Specialized method to map unstructured grid cells to span space. Uses
// GetCellPoints() to retrieve points defining the cell.
template <typename TS>
struct MapUGridToSpanSpace
{
  svtkInternalSpanSpace* SpanSpace;
  svtkUnstructuredGrid* Grid;
  TS* Scalars;

  MapUGridToSpanSpace(svtkInternalSpanSpace* ss, svtkUnstructuredGrid* ds, TS* s)
    : SpanSpace(ss)
    , Grid(ds)
    , Scalars(s)
  {
  }

  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    svtkUnstructuredGrid* grid = this->Grid;
    TS* scalars = this->Scalars;
    svtkIdType i, npts;
    const svtkIdType* pts;
    double s, sMin, sMax;

    for (; cellId < endCellId; ++cellId)
    {
      sMin = SVTK_DOUBLE_MAX;
      sMax = SVTK_DOUBLE_MIN;
      // A faster version of GetCellPoints()
      grid->GetCellPoints(cellId, npts, pts);
      for (i = 0; i < npts; i++)
      {
        s = static_cast<double>(scalars[pts[i]]);
        sMin = (s < sMin ? s : sMin);
        sMax = (s > sMax ? s : sMax);
      } // for all cell scalars
      // Compute span space id, and prepare to map
      this->SpanSpace->SetSpanPoint(cellId, sMin, sMax);
    } // for all cells in this thread
  }

  static void Execute(svtkIdType numCells, svtkInternalSpanSpace* ss, svtkUnstructuredGrid* ds, TS* s)
  {
    MapUGridToSpanSpace map(ss, ds, s);
    svtkSMPTools::For(0, numCells, map);
  }
}; // MapUGridToSpanSpace

} // anonymous namespace

//---The SVTK Classes proper------------------------------------------------------

svtkStandardNewMacro(svtkSpanSpace);

//-----------------------------------------------------------------------------
// Instantiate empty span space object.
svtkSpanSpace::svtkSpanSpace()
{
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->ComputeScalarRange = true;
  this->Resolution = 100;
  this->ComputeResolution = true;
  this->NumberOfCellsPerBucket = 5;
  this->SpanSpace = nullptr;
  this->RMin[0] = this->RMin[1] = 0;
  this->RMax[0] = this->RMax[1] = 0;
  this->BatchSize = 100;
}

//-----------------------------------------------------------------------------
svtkSpanSpace::~svtkSpanSpace()
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
// Shallow copy enough information for a clone to produce the same result on
// the same data.
void svtkSpanSpace::ShallowCopy(svtkScalarTree* stree)
{
  svtkSpanSpace* ss = svtkSpanSpace::SafeDownCast(stree);
  if (ss != nullptr)
  {
    this->SetScalarRange(ss->GetScalarRange());
    this->SetComputeScalarRange(ss->GetComputeScalarRange());
    this->SetResolution(ss->GetResolution());
    this->SetComputeResolution(ss->GetComputeResolution());
    this->SetNumberOfCellsPerBucket(ss->GetNumberOfCellsPerBucket());
  }
  // Now do superclass
  this->Superclass::ShallowCopy(stree);
}

//-----------------------------------------------------------------------------
// Frees memory and resets object as appropriate.
void svtkSpanSpace::Initialize()
{
  if (this->SpanSpace)
  {
    delete this->SpanSpace;
    this->SpanSpace = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Construct the scalar tree / span space from the dataset
// provided. Checks build times and modified time from input and
// reconstructs the tree if necessary.
void svtkSpanSpace::BuildTree()
{
  svtkIdType numCells;

  // Check input...see whether we have to rebuild
  //
  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    svtkErrorMacro(<< "No data to build tree with");
    return;
  }

  if (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }

  svtkDebugMacro(<< "Building span space...");

  // If no scalars set then try and grab them from dataset
  if (!this->Scalars)
  {
    this->SetScalars(this->DataSet->GetPointData()->GetScalars());
  }
  if (!this->Scalars)
  {
    svtkErrorMacro(<< "No scalar data to build trees with");
    return;
  }

  // We need a scalar range for the scalars. Do this in parallel for a small
  // boost in performance.
  double range[2];
  void* scalars = this->Scalars->GetVoidPointer(0);

  if (this->ComputeScalarRange)
  {
    switch (this->Scalars->GetDataType())
    {
      svtkTemplateMacro(
        ComputeRange<SVTK_TT>::Execute(this->Scalars->GetNumberOfTuples(), (SVTK_TT*)scalars, range));
    }
    this->ScalarRange[0] = range[0];
    this->ScalarRange[1] = range[1];
  }
  else
  {
    range[0] = this->ScalarRange[0];
    range[1] = this->ScalarRange[1];
  }

  double R = range[1] - range[0];
  if (R <= 0.0)
  {
    svtkErrorMacro(<< "Bad scalar range");
    return;
  }

  // Prepare to process scalars
  this->Initialize(); // clears out old span space arrays

  // The first pass loops over all cells, mapping them into span space
  // (i.e., an integer id into a gridded span space). Later this id will
  // be used to sort the cells across the span space, so that cells
  // can be processed in order by different threads.
  if (this->ComputeResolution)
  {
    this->Resolution = static_cast<svtkIdType>(
      sqrt(static_cast<double>(numCells) / static_cast<double>(this->NumberOfCellsPerBucket)));
    this->Resolution =
      (this->Resolution < 100 ? 100 : (this->Resolution > 10000 ? 10000 : this->Resolution));
  }
  this->SpanSpace = new svtkInternalSpanSpace(this->Resolution, range[0], range[1], numCells);

  // Acclerated span space construction (for unstructured grids).  Templated
  // over scalar type; direct access to svtkUnstructuredGrid innards.
  svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(this->DataSet);
  if (ugrid != nullptr)
  {
    switch (this->Scalars->GetDataType())
    {
      svtkTemplateMacro(
        MapUGridToSpanSpace<SVTK_TT>::Execute(numCells, this->SpanSpace, ugrid, (SVTK_TT*)scalars));
    }
  }

  // Generic, threaded processing of cells to produce span space.
  else
  {
    MapToSpanSpace::Execute(numCells, this->SpanSpace, this->DataSet, this->Scalars);
  }

  // Now sort and build span space
  this->SpanSpace->Build();

  // Update our build time
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Begin to traverse the cells based on a scalar value. Returned cells
// will have scalar values that span the scalar value specified.
void svtkSpanSpace::InitTraversal(double scalarValue)
{
  this->BuildTree();
  this->ScalarValue = scalarValue;

  // Find the rectangle in span space that spans the isovalue
  this->SpanSpace->GetSpanRectangle(scalarValue, this->RMin, this->RMax);

  // Initiate the serial looping over all span rows
  this->CurrentRow = this->RMin[1];
  this->CurrentSpan = this->SpanSpace->GetCellsInSpan(
    this->CurrentRow, this->RMin, this->RMax, this->CurrentNumCells);
  this->CurrentIdx = 0; // beginning of current span row
}

//-----------------------------------------------------------------------------
// Return the next cell that may contain scalar value specified to
// initialize traversal. The value nullptr is returned if the list is
// exhausted. Make sure that InitTraversal() has been invoked first or
// you'll get erratic behavior. This is serial traversal.
svtkCell* svtkSpanSpace::GetNextCell(
  svtkIdType& cellId, svtkIdList*& cellPts, svtkDataArray* cellScalars)
{
  // Where are we in the current span space row? If at the end, need to get the
  // next row (or return if the last row)
  while (this->CurrentIdx >= this->CurrentNumCells)
  {
    this->CurrentRow++;
    if (this->CurrentRow >= this->RMax[1])
    {
      return nullptr;
    }
    else
    {
      this->CurrentSpan = this->SpanSpace->GetCellsInSpan(
        this->CurrentRow, this->RMin, this->RMax, this->CurrentNumCells);
      this->CurrentIdx = 0; // beginning of row
    }
  }

  // If here then get the next cell
  svtkIdType numScalars;
  svtkCell* cell;
  cellId = this->CurrentSpan[this->CurrentIdx++];
  cell = this->DataSet->GetCell(cellId);
  cellPts = cell->GetPointIds();
  numScalars = cellPts->GetNumberOfIds();
  cellScalars->SetNumberOfTuples(numScalars);
  this->Scalars->GetTuples(cellPts, cellScalars);

  return cell;
}

//-----------------------------------------------------------------------------
// Note the cell ids are copied into memory (CandidateCells) from
// which batches are created. This is done for load balancing
// purposes. The span space can often aggregate many cells in just a
// few bins; meaning that batches cannot just be span rows if the work
// is to shared across many threads.
svtkIdType svtkSpanSpace::GetNumberOfCellBatches(double scalarValue)
{
  // Make sure tree is built, modified time will prevent reexecution.
  this->BuildTree();
  this->ScalarValue = scalarValue;

  // Find the rectangle in span space that spans the isovalue
  svtkInternalSpanSpace* sp = this->SpanSpace;
  ;
  sp->GetSpanRectangle(scalarValue, this->RMin, this->RMax);

  // Loop over each span row to count total memory allocation required.
  svtkIdType numCandidates = 0;
  svtkIdType row, *span, idx, numCells;
  for (row = this->RMin[1]; row < this->RMax[1]; ++row)
  {
    sp->GetCellsInSpan(row, this->RMin, this->RMax, numCells);
    numCandidates += numCells;
  } // for all rows in span rectangle

  // Allocate list of candidate cells. Cache memory to avoid
  // reallocation if possible.
  if (sp->CandidateCells != nullptr && numCandidates > sp->NumCandidates)
  {
    delete[] sp->CandidateCells;
    sp->CandidateCells = nullptr;
  }
  sp->NumCandidates = numCandidates;
  if (numCandidates > 0 && sp->CandidateCells == nullptr)
  {
    sp->CandidateCells = new svtkIdType[sp->NumCandidates];
  }

  // Now copy cells into the allocated memory. This could be done in
  // parallel (a parallel write - TODO) but probably wouldn't provide
  // much of a boost.
  numCandidates = 0;
  for (row = this->RMin[1]; row < this->RMax[1]; ++row)
  {
    span = sp->GetCellsInSpan(row, this->RMin, this->RMax, numCells);
    for (idx = 0; idx < numCells; ++idx)
    {
      sp->CandidateCells[numCandidates++] = span[idx];
    }
  } // for all rows in span rectangle

  // Watch for boundary conditions. Return BatchSize cells to a batch.
  if (sp->NumCandidates < 1)
  {
    return 0;
  }
  else
  {
    return (((sp->NumCandidates - 1) / this->BatchSize) + 1);
  }
}

//-----------------------------------------------------------------------------
// Call after GetNumberOfCellBatches(isoValue)
const svtkIdType* svtkSpanSpace::GetCellBatch(svtkIdType batchNum, svtkIdType& numCells)
{
  // Make sure that everything is hunky dory
  svtkInternalSpanSpace* sp = this->SpanSpace;
  ;
  svtkIdType pos = batchNum * this->BatchSize;
  if (sp->NumCells < 1 || !sp->CandidateCells || pos >= sp->NumCandidates)
  {
    numCells = 0;
    return nullptr;
  }

  // Return a batch, or if near the end of the candidate list,
  // the remainder batch.
  if ((sp->NumCandidates - pos) >= this->BatchSize)
  {
    numCells = this->BatchSize;
  }
  else
  {
    numCells = sp->NumCandidates % this->BatchSize;
  }

  return sp->CandidateCells + pos;
}

//-----------------------------------------------------------------------------
void svtkSpanSpace::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << "," << this->ScalarRange[1] << ")\n";
  os << indent << "Compute Scalar Range: " << (this->ComputeScalarRange ? "On\n" : "Off\n");
  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Compute Resolution: " << (this->ComputeResolution ? "On\n" : "Off\n");
  os << indent << "Number of Cells Per Bucket: " << this->NumberOfCellsPerBucket << "\n";
}
