/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DLinearGridCrinkleExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtk3DLinearGridCrinkleExtractor.h"

#include "svtk3DLinearGridInternal.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticCellLinksTemplate.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <atomic>
#include <vector>

svtkStandardNewMacro(svtk3DLinearGridCrinkleExtractor);
svtkCxxSetObjectMacro(svtk3DLinearGridCrinkleExtractor, ImplicitFunction, svtkImplicitFunction);

//-----------------------------------------------------------------------------
// Macros immediately below are just used to make code easier to
// read. Invokes functor _op _num times depending on serial (_seq==1) or
// parallel processing mode. The _REDUCE_ version is used to called functors
// with a Reduce() method).
#define EXECUTE_SMPFOR(_seq, _num, _op)                                                            \
  if (!_seq)                                                                                       \
  {                                                                                                \
    svtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op(0, _num);                                                                                  \
  }

#define EXECUTE_REDUCED_SMPFOR(_seq, _num, _op, _nt)                                               \
  if (!_seq)                                                                                       \
  {                                                                                                \
    svtkSMPTools::For(0, _num, _op);                                                                \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    _op.Initialize();                                                                              \
    _op(0, _num);                                                                                  \
    _op.Reduce();                                                                                  \
  }                                                                                                \
  _nt = _op.NumThreadsUsed;

namespace
{ // anonymous

//========================= Quick implicit function cell selection =============

// Compute an array that classifies each point with respect to the current
// implicit function (i.e. above the function(=2), below the function(=1), on
// the function(=0)).  InOutArray is allocated here and should be deleted by
// the invoking code. InOutArray is an unsigned char array to simplify bit
// fiddling later on (i.e., Intersects() method). A fast path is available
// for svtkPlane implicit functions.
//
// The reason we compute this unsigned char array as compared to an array of
// function values is to reduce the amount of memory used, and writing to
// memory, since these are significant costs for large data.

// Templated for explicit point representations of real type
template <typename TP>
struct PlaneClassifyPoints;
template <typename TP>
struct FunctionClassifyPoints;

// General classification capability
struct Classify
{
  unsigned char* InOutArray;

  Classify(svtkPoints* pts) { this->InOutArray = new unsigned char[pts->GetNumberOfPoints()]; }

  // Check if a list of points intersects the plane
  static bool Intersects(const unsigned char* inout, svtkIdType npts, const svtkIdType* pts)
  {
    unsigned char onOneSideOfPlane = inout[pts[0]];
    for (svtkIdType i = 1; onOneSideOfPlane && i < npts; ++i)
    {
      onOneSideOfPlane &= inout[pts[i]];
    }
    return (!onOneSideOfPlane);
  }
};

// Faster path for svtkPlane
template <typename TP>
struct PlaneClassifyPoints : public Classify
{
  TP* Points;
  double Origin[3];
  double Normal[3];

  PlaneClassifyPoints(svtkPoints* pts, svtkPlane* plane)
    : Classify(pts)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
    plane->GetOrigin(this->Origin);
    plane->GetNormal(this->Normal);
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    double *n = this->Normal, *o = this->Origin;
    TP* pts = this->Points + 3 * ptId;
    unsigned char* ioa = this->InOutArray + ptId;
    for (; ptId < endPtId; ++ptId)
    {
      // Access each point
      p[0] = static_cast<double>(*pts);
      ++pts;
      p[1] = static_cast<double>(*pts);
      ++pts;
      p[2] = static_cast<double>(*pts);
      ++pts;

      // Evaluate position of the point with the plane. Invoke inline,
      // non-virtual version of evaluate method.
      eval = svtkPlane::Evaluate(n, o, p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *ioa++ = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
  }
};

// General path for svtkImplicitFunction
template <typename TP>
struct FunctionClassifyPoints : public Classify
{
  TP* Points;
  svtkImplicitFunction* Function;

  FunctionClassifyPoints(svtkPoints* pts, svtkImplicitFunction* f)
    : Classify(pts)
    , Function(f)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double p[3], zero = double(0), eval;
    TP* pts = this->Points + 3 * ptId;
    unsigned char* ioa = this->InOutArray + ptId;
    for (; ptId < endPtId; ++ptId)
    {
      // Access each point
      p[0] = static_cast<double>(*pts);
      ++pts;
      p[1] = static_cast<double>(*pts);
      ++pts;
      p[2] = static_cast<double>(*pts);
      ++pts;

      // Evaluate position of the point wrt the implicit function. This
      // call better be thread safe.
      eval = this->Function->FunctionValue(p);

      // Point is either above(=2), below(=1), or on(=0) the plane.
      *ioa++ = (eval > zero ? 2 : (eval < zero ? 1 : 0));
    }
  }
};

// Base class for extracting cells and points from the input
// svtkUnstructuredGrid.
struct ExtractCellsBase
{
  typedef std::vector<svtkIdType> CellArrayType;
  typedef std::vector<svtkIdType> OriginCellType;
  typedef std::vector<unsigned char> CellTypesType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    CellArrayType LocalCells;
    OriginCellType LocalOrigins;
    CellTypesType LocalTypes;
    svtkIdType LocalNumCells;
    CellIter LocalCellIter;

    LocalDataType()
      : LocalNumCells(0)
    {
    }
  };

  const unsigned char* InOut;
  CellIter* Iter;
  svtkIdType InputNumPts;
  svtkIdType OutputNumPts;
  svtkIdType OutputNumCells;
  svtkIdType TotalSize;
  svtkUnstructuredGrid* Grid;
  svtkCellArray* Cells;
  bool CopyPointData;
  bool CopyCellData;
  svtkIdType* PointMap;
  svtkIdType* CellMap;
  int NumThreadsUsed;
  svtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractCellsBase(svtkIdType inNumPts, CellIter* c, unsigned char* inout, svtkUnstructuredGrid* grid,
    svtkCellArray* cells, bool copyPtData, bool copyCellData)
    : InOut(inout)
    , Iter(c)
    , InputNumPts(inNumPts)
    , OutputNumPts(0)
    , OutputNumCells(0)
    , TotalSize(0)
    , Grid(grid)
    , Cells(cells)
    , CopyPointData(copyPtData)
    , CopyCellData(copyCellData)
    , PointMap(nullptr)
    , CellMap(nullptr)
    , NumThreadsUsed(0)
  {
  }

  // Set up the iteration process
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

}; // ExtractCellsBase

// Traverse all cells and extract intersected cells
struct ExtractCells : public ExtractCellsBase
{
  ExtractCells(svtkIdType inNumPts, CellIter* c, unsigned char* inout, svtkUnstructuredGrid* grid,
    svtkCellArray* cells, bool copyPtData, bool copyCellData)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, copyPtData, copyCellData)
  {
  }

  void Initialize() { this->ExtractCellsBase::Initialize(); }

  // operator() method extracts cells
  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lCells = localData.LocalCells;
    auto& lOrigins = localData.LocalOrigins;
    auto& lTypes = localData.LocalTypes;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    const unsigned char* inout = this->InOut;
    svtkIdType& lNumCells = localData.LocalNumCells;
    svtkIdType npts;

    for (; cellId < endCellId; ++cellId)
    {
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (Classify::Intersects(inout, npts, c))
      {
        ++lNumCells;
        lTypes.emplace_back(cellIter->GetCellType(cellId));
        lCells.emplace_back(npts);
        const svtkIdType* pts = cellIter->GetCellIds(cellId);
        for (auto i = 0; i < npts; ++i)
        {
          lCells.emplace_back(pts[i]);
        }
        if (this->CopyCellData)
        {
          lOrigins.emplace_back(cellId); // to support cell data copying
        }
      }                     // if implicit function intersects
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite local thread data. Basically build the output unstructured grid.
  void Reduce()
  {
    // Count the number of cells, and the number of threads used. Figure out the total
    // length of the cell array.
    svtkIdType numCells = 0;
    svtkIdType size = 0;
    for (const auto& threadData : this->LocalData)
    {
      numCells += threadData.LocalNumCells;
      size += static_cast<svtkIdType>(threadData.LocalCells.size());
      this->NumThreadsUsed++;
    }
    this->OutputNumCells = numCells;
    this->TotalSize = size;

    // Now allocate the cell array, offset array, and cell types array.
    this->Cells->AllocateExact(numCells, size - numCells);
    svtkNew<svtkUnsignedCharArray> cellTypes;
    unsigned char* ctptr = static_cast<unsigned char*>(cellTypes->WriteVoidPointer(0, numCells));

    // If cell data is requested, roll up generating cell ids
    svtkIdType* cellMap = nullptr;
    if (this->CopyCellData)
    {
      this->CellMap = cellMap = new svtkIdType[numCells];
    }

    // Now composite the cell-related information
    for (const auto& threadData : this->LocalData)
    {
      const auto& lCells = threadData.LocalCells;
      const auto& lTypes = threadData.LocalTypes;
      const auto& lOrigins = threadData.LocalOrigins;
      numCells = threadData.LocalNumCells;

      this->Cells->AppendLegacyFormat(lCells.data(), static_cast<svtkIdType>(lCells.size()));
      ctptr = std::copy_n(lTypes.cbegin(), numCells, ctptr);
      if (this->CopyCellData)
      {
        cellMap = std::copy_n(lOrigins.cbegin(), numCells, cellMap);
      }
    } // for this thread

    // Define the grid
    this->Grid->SetCells(cellTypes, this->Cells);

  } // Reduce

}; // ExtractCells

// Traverse all cells to extract intersected cells and remapped points
struct ExtractPointsAndCells : public ExtractCellsBase
{
  ExtractPointsAndCells(svtkIdType inNumPts, CellIter* c, unsigned char* inout,
    svtkUnstructuredGrid* grid, svtkCellArray* cells, bool copyPtData, bool copyCellData)
    : ExtractCellsBase(inNumPts, c, inout, grid, cells, copyPtData, copyCellData)
  {
    this->PointMap = new svtkIdType[inNumPts];
    std::fill_n(this->PointMap, inNumPts, (-1));
  }

  void Initialize() { this->ExtractCellsBase::Initialize(); }

  // operator() method identifies cells and points to extract
  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lCells = localData.LocalCells;
    auto& lOrigins = localData.LocalOrigins;
    auto& lTypes = localData.LocalTypes;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    const unsigned char* inout = this->InOut;
    svtkIdType& lNumCells = localData.LocalNumCells;
    svtkIdType npts;
    svtkIdType* pointMap = this->PointMap;

    for (; cellId < endCellId; ++cellId)
    {
      // Does the implicit function cut this cell?
      npts = cellIter->NumVerts;
      if (Classify::Intersects(inout, npts, c))
      {
        ++lNumCells;
        lTypes.emplace_back(cellIter->GetCellType(cellId));
        lCells.emplace_back(npts);
        const svtkIdType* pts = cellIter->GetCellIds(cellId);
        for (auto i = 0; i < npts; ++i)
        {
          pointMap[pts[i]] = 1; // this point is used
          lCells.emplace_back(pts[i]);
        }
        if (this->CopyCellData)
        {
          lOrigins.emplace_back(cellId); // to support cell data copying
        }
      }                     // if implicit function intersects
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite local thread data. Basically build the output unstructured grid.
  void Reduce()
  {
    // Generate point map
    svtkIdType globalPtId = 0;
    svtkIdType* ptMap = this->PointMap;
    for (auto ptId = 0; ptId < this->InputNumPts; ++ptId)
    {
      if (this->PointMap[ptId] > 0)
      {
        ptMap[ptId] = globalPtId++;
      }
    }
    this->OutputNumPts = globalPtId;

    // Count the number of cells, and the number of threads used. Figure out the total
    // length of the cell array.
    svtkIdType numCells = 0;
    svtkIdType size = 0;
    for (const auto& threadData : this->LocalData)
    {
      numCells += threadData.LocalNumCells;
      size += static_cast<svtkIdType>(threadData.LocalCells.size());
      this->NumThreadsUsed++;
    }
    this->OutputNumCells = numCells;
    this->TotalSize = size;

    // Now allocate the cell array, offset array, and cell types array.
    this->Cells->AllocateExact(numCells, size - numCells);
    svtkNew<svtkUnsignedCharArray> cellTypes;
    unsigned char* ctptr = static_cast<unsigned char*>(cellTypes->WriteVoidPointer(0, numCells));

    // If cell data is requested, roll up generating cell ids
    svtkIdType* cellMap = nullptr;
    this->CellMap = cellMap = new svtkIdType[numCells];

    // Now composite the cell-related information
    for (const auto& threadData : this->LocalData)
    {
      const auto& lCells = threadData.LocalCells;
      const auto& lTypes = threadData.LocalTypes;
      const auto& lOrigins = threadData.LocalOrigins;
      numCells = threadData.LocalNumCells;

      ctptr = std::copy_n(lTypes.cbegin(), numCells, ctptr);
      if (this->CopyCellData)
      {
        cellMap = std::copy_n(lOrigins.cbegin(), numCells, cellMap);
      }

      // Need to do this in a loop since the pointIds are mapped through ptMap:
      auto threadCells = lCells.cbegin();
      for (auto i = 0; i < numCells; ++i)
      {
        const svtkIdType npts = *threadCells++;
        this->Cells->InsertNextCell(static_cast<int>(npts));
        for (auto j = 0; j < npts; ++j)
        {
          this->Cells->InsertCellPoint(ptMap[*threadCells++]);
        }
      } // over all the cells in this thread
    }   // for this thread

    // Define the grid
    this->Grid->SetCells(cellTypes, this->Cells);

  } // Reduce

}; // ExtractPointsAndCells

// Copy cell data from input to output
struct CopyCellAttributes
{
  ArrayList* Arrays;
  const svtkIdType* CellMap;

  CopyCellAttributes(ArrayList* arrays, const svtkIdType* cellMap)
    : Arrays(arrays)
    , CellMap(cellMap)
  {
  }

  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    for (; cellId < endCellId; ++cellId)
    {
      this->Arrays->Copy(this->CellMap[cellId], cellId);
    }
  }
};

// Generate point coordinates
template <typename TPIn, typename TPOut>
struct GeneratePoints
{
  const TPIn* InPts;
  const svtkIdType* PointMap;
  TPOut* OutPts;

  GeneratePoints(TPIn* inPts, svtkIdType* ptMap, TPOut* outPts)
    : InPts(inPts)
    , PointMap(ptMap)
    , OutPts(outPts)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const TPIn* p = this->InPts + 3 * ptId;
    const svtkIdType* ptMap = this->PointMap;
    TPOut *outPts = this->OutPts, *x;

    for (; ptId < endPtId; ++ptId, p += 3)
    {
      if (ptMap[ptId] >= 0)
      {
        x = outPts + 3 * ptMap[ptId];
        *x++ = static_cast<TPOut>(p[0]);
        *x++ = static_cast<TPOut>(p[1]);
        *x = static_cast<TPOut>(p[2]);
      }
    }
  }
};

// Copy point data from input to output.
struct CopyPointAttributes
{
  ArrayList* Arrays;
  const svtkIdType* PointMap;

  CopyPointAttributes(ArrayList* arrays, const svtkIdType* ptMap)
    : Arrays(arrays)
    , PointMap(ptMap)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const svtkIdType* ptMap = this->PointMap;
    for (; ptId < endPtId; ++ptId)
    {
      if (ptMap[ptId] >= 0)
      {
        this->Arrays->Copy(ptId, ptMap[ptId]);
      }
    }
  }
};

} // anonymous namespace

//-----------------------------------------------------------------------------
// Construct an instance of the class.
svtk3DLinearGridCrinkleExtractor::svtk3DLinearGridCrinkleExtractor()
{
  this->ImplicitFunction = nullptr;
  this->CopyPointData = true;
  this->CopyCellData = false;
  this->RemoveUnusedPoints = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
}

//-----------------------------------------------------------------------------
svtk3DLinearGridCrinkleExtractor::~svtk3DLinearGridCrinkleExtractor()
{
  this->SetImplicitFunction(nullptr);
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If the implicit function
// definition is modified, then this object is modified as well.
svtkMTimeType svtk3DLinearGridCrinkleExtractor::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ImplicitFunction != nullptr)
  {
    svtkMTimeType mTime2 = this->ImplicitFunction->GetMTime();
    return (mTime2 > mTime ? mTime2 : mTime);
  }
  else
  {
    return mTime;
  }
}

//-----------------------------------------------------------------------------
// Specialized implicit function extraction filter to handle unstructured
// grids with 3D linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int svtk3DLinearGridCrinkleExtractor::ProcessPiece(
  svtkUnstructuredGrid* input, svtkImplicitFunction* f, svtkUnstructuredGrid* grid)
{
  if (input == nullptr || f == nullptr || grid == nullptr)
  {
    // Not really an error
    return 1;
  }

  // Make sure there is input data to process
  svtkPoints* inPts = input->GetPoints();
  svtkIdType numPts = 0;
  if (inPts)
  {
    numPts = inPts->GetNumberOfPoints();
  }
  svtkCellArray* cells = input->GetCells();
  svtkIdType numCells = 0;
  if (cells)
  {
    numCells = cells->GetNumberOfCells();
  }
  if (numPts <= 0 || numCells <= 0)
  {
    svtkLog(INFO, "Empty input");
    return 0;
  }

  // Check the input point type. Only real types are supported.
  int inPtsType = inPts->GetDataType();
  if ((inPtsType != SVTK_FLOAT && inPtsType != SVTK_DOUBLE))
  {
    svtkLog(ERROR, "Input point type not supported");
    return 0;
  }

  // Output cells go here.
  svtkCellArray* newCells = svtkCellArray::New();

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Classify the cell points based on the specified implicit function. A
  // fast path is available for planes.
  unsigned char* inout = nullptr;
  int ptsType = inPts->GetDataType();
  if (svtkPlane::SafeDownCast(f) != nullptr)
  { // plane fast path
    if (ptsType == SVTK_FLOAT)
    {
      PlaneClassifyPoints<float> classify(inPts, static_cast<svtkPlane*>(f));
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
    else if (ptsType == SVTK_DOUBLE)
    {
      PlaneClassifyPoints<double> classify(inPts, static_cast<svtkPlane*>(f));
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
  }
  else
  { // general implicit function fast path
    if (ptsType == SVTK_FLOAT)
    {
      FunctionClassifyPoints<float> classify(inPts, f);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
    else if (ptsType == SVTK_DOUBLE)
    {
      FunctionClassifyPoints<double> classify(inPts, f);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, classify);
      inout = classify.InOutArray;
    }
  }

  // Depending on whether we are going to eliminate unused points, use
  // different extraction techniques. There is a large performance
  // difference if points are compacted.
  svtkIdType outNumCells = 0;
  svtkIdType* cellMap = nullptr;
  svtkIdType outNumPts = 0;
  svtkIdType* ptMap = nullptr;
  svtkPointData* inPD = input->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  if (!this->RemoveUnusedPoints)
  {
    ExtractCells extract(
      numPts, cellIter, inout, grid, newCells, this->CopyPointData, this->CopyCellData);
    EXECUTE_REDUCED_SMPFOR(
      this->SequentialProcessing, numCells, extract, this->NumberOfThreadsUsed);

    outNumCells = extract.OutputNumCells;
    cellMap = extract.CellMap;

    grid->SetPoints(inPts);
    if (this->CopyPointData)
    {
      svtkPointData* outPD = grid->GetPointData();
      outPD->PassData(inPD);
    }
  }

  else
  {
    ExtractPointsAndCells extract(
      numPts, cellIter, inout, grid, newCells, this->CopyPointData, this->CopyCellData);
    EXECUTE_REDUCED_SMPFOR(
      this->SequentialProcessing, numCells, extract, this->NumberOfThreadsUsed);

    outNumPts = extract.OutputNumPts;
    ptMap = extract.PointMap;

    outNumCells = extract.OutputNumCells;
    cellMap = extract.CellMap;
  }

  // Copy cell data if requested
  if (this->CopyCellData)
  {
    svtkCellData* outCD = grid->GetCellData();
    ArrayList arrays;
    outCD->CopyAllocate(inCD, outNumCells);
    arrays.AddArrays(outNumCells, inCD, outCD);
    CopyCellAttributes copyCellData(&arrays, cellMap);
    EXECUTE_SMPFOR(this->SequentialProcessing, outNumCells, copyCellData);
    delete[] cellMap;
  }

  if (this->RemoveUnusedPoints)
  {
    // Create the output points if not passing through. Only real types are
    // supported. Use the point map to create them.
    int inType = inPts->GetDataType(), outType;
    void *inPtr, *outPtr;
    svtkPoints* outPts = svtkPoints::New();
    if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
    {
      outType = inType;
    }
    else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
    {
      outType = SVTK_FLOAT;
    }
    else // if(this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
    {
      outType = SVTK_DOUBLE;
    }
    outPts->SetDataType(outType);
    outPts->SetNumberOfPoints(outNumPts);

    // Generate points using the point map
    inPtr = inPts->GetData()->GetVoidPointer(0);
    outPtr = outPts->GetData()->GetVoidPointer(0);
    if (inType == SVTK_DOUBLE && outType == SVTK_DOUBLE)
    {
      GeneratePoints<double, double> generatePts((double*)inPtr, ptMap, (double*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else if (inType == SVTK_FLOAT && outType == SVTK_FLOAT)
    {
      GeneratePoints<float, float> generatePts((float*)inPtr, ptMap, (float*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else if (inType == SVTK_DOUBLE && outType == SVTK_FLOAT)
    {
      GeneratePoints<double, float> generatePts((double*)inPtr, ptMap, (float*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    else // if ( inType == SVTK_FLOAT && outType == SVTK_DOUBLE )
    {
      GeneratePoints<float, double> generatePts((float*)inPtr, ptMap, (double*)outPtr);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, generatePts);
    }
    grid->SetPoints(outPts);
    outPts->Delete();

    // Use the point map to copy point data if desired
    if (this->CopyPointData)
    {
      svtkPointData* outPD = grid->GetPointData();
      ArrayList arrays;
      outPD->CopyAllocate(inPD, outNumPts);
      arrays.AddArrays(outNumPts, inPD, outPD);
      CopyPointAttributes copyPointData(&arrays, ptMap);
      EXECUTE_SMPFOR(this->SequentialProcessing, numPts, copyPointData);
      delete[] ptMap;
    }
  }

  // Report the results of execution
  svtkLog(INFO,
    "Extracted: " << grid->GetNumberOfPoints() << " points, " << grid->GetNumberOfCells()
                  << " cells");

  // Clean up
  if (inout != nullptr)
  {
    delete[] inout;
  }
  delete cellIter;
  newCells->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// The output dataset type varies depending on the input type.
int svtk3DLinearGridCrinkleExtractor::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataObject* inputDO = svtkDataObject::GetData(inputVector[0], 0);
  svtkDataObject* outputDO = svtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (svtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    if (svtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkUnstructuredGrid::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (svtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a svtkMultiBlockDataSet as output;
    if (svtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkMultiBlockDataSet::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  svtkLog(ERROR, "Not sure what type of output to create!");
  return 0;
}

//-----------------------------------------------------------------------------
// Specialized extraction filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int svtk3DLinearGridCrinkleExtractor::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input and output
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkUnstructuredGrid* inputGrid =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* outputGrid =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkCompositeDataSet* inputCDS =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkMultiBlockDataSet* outputMBDS =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputGrid == nullptr) &&
    (inputCDS == nullptr || outputMBDS == nullptr))
  {
    return 0;
  }

  // Need an implicit function to do the cutting
  svtkImplicitFunction* f = this->ImplicitFunction;
  if (!f)
  {
    svtkLog(ERROR, "Implicit function not defined");
    return 0;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output svtkUnstructuredGrid.
  if (inputGrid)
  {
    this->ProcessPiece(inputGrid, f, outputGrid);
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a svtkGrid that is added to
  // the output multiblock dataset.
  else
  {
    svtkUnstructuredGrid* grid;
    svtkUnstructuredGrid* output;
    outputMBDS->CopyStructure(inputCDS);
    svtkSmartPointer<svtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = svtkUnstructuredGrid::SafeDownCast(ds)))
      {
        output = svtkUnstructuredGrid::New();
        this->ProcessPiece(grid, f, output);
        outputMBDS->SetDataSet(inIter, output);
        output->Delete();
      }
      else
      {
        svtkLog(INFO, << "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtk3DLinearGridCrinkleExtractor::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtk3DLinearGridCrinkleExtractor::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
bool svtk3DLinearGridCrinkleExtractor::CanFullyProcessDataObject(svtkDataObject* object)
{
  auto ug = svtkUnstructuredGrid::SafeDownCast(object);
  auto cd = svtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    // Get list of cell types in the unstructured grid
    svtkNew<svtkCellTypes> cellTypes;
    ug->GetCellTypes(cellTypes);
    for (svtkIdType i = 0; i < cellTypes->GetNumberOfTypes(); ++i)
    {
      unsigned char cellType = cellTypes->GetCellType(i);
      if (cellType != SVTK_VOXEL && cellType != SVTK_TETRA && cellType != SVTK_HEXAHEDRON &&
        cellType != SVTK_WEDGE && cellType != SVTK_PYRAMID)
      {
        // Unsupported cell type, can't process data
        return false;
      }
    }

    // All cell types are supported, can process data.
    return true;
  }
  else if (cd)
  {
    bool supported = true;
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto leafDS = iter->GetCurrentDataObject();
      if (!CanFullyProcessDataObject(leafDS))
      {
        supported = false;
        break;
      }
    }
    return supported;
  }

  return false; // not a svtkUnstructuredGrid nor a composite dataset
}

//-----------------------------------------------------------------------------
int svtk3DLinearGridCrinkleExtractor::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void svtk3DLinearGridCrinkleExtractor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";

  os << indent << "Copy Point Data: " << (this->CopyPointData ? "true\n" : "false\n");
  os << indent << "Copy Cell Data: " << (this->CopyCellData ? "true\n" : "false\n");

  os << indent << "RemoveUnusedPoints: " << (this->RemoveUnusedPoints ? "true\n" : "false\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
