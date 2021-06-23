/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContour3DLinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContour3DLinearGrid.h"

#include "svtk3DLinearGridInternal.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkContourValues.h"
#include "svtkDataArrayRange.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkHexahedron.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPyramid.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkSpanSpace.h"
#include "svtkStaticCellLinksTemplate.h"
#include "svtkStaticEdgeLocatorTemplate.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <utility> //make_pair

svtkStandardNewMacro(svtkContour3DLinearGrid);
svtkCxxSetObjectMacro(svtkContour3DLinearGrid, ScalarTree, svtkScalarTree);

//-----------------------------------------------------------------------------
// Classes to support threaded execution. Note that there are different
// strategies implemented here: 1) a fast path that just produces output
// triangles and points, and 2) more general approach that supports point
// merging, field interpolation, and/or normal generation. There is also some
// cell-related machinery supporting faster contouring. Finally, a scalar
// tree can be used to accelerate repeated contouring.

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
{

//========================= FAST PATH =========================================
// Perform the contouring operation without merging coincident points. There is
// a fast path with and without a scalar tree.
template <typename TIP, typename TOP, typename TS>
struct ContourCellsBase
{
  typedef std::vector<TOP> LocalPtsType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread into a
  // single svtkPolyData output.
  struct LocalDataType
  {
    LocalPtsType LocalPts;
    CellIter LocalCellIter;
    LocalDataType() { this->LocalPts.reserve(2048); }
  };

  CellIter* Iter;
  const TIP* InPts;
  const TS* Scalars;
  double Value;
  svtkPoints* NewPts;
  svtkCellArray* NewPolys;

  // Keep track of generated points and triangles on a per thread basis
  svtkSMPThreadLocal<LocalDataType> LocalData;

  // Related to the compositing Reduce() method
  svtkIdType NumPts;
  svtkIdType NumTris;
  int NumThreadsUsed;
  svtkIdType TotalPts;  // the total points thus far (support multiple contours)
  svtkIdType TotalTris; // the total triangles thus far (support multiple contours)
  svtkTypeBool Sequential;

  ContourCellsBase(TIP* inPts, CellIter* iter, TS* s, double value, svtkPoints* outPts,
    svtkCellArray* tris, svtkIdType totalPts, svtkIdType totalTris, svtkTypeBool seq)
    : Iter(iter)
    , InPts(inPts)
    , Scalars(s)
    , Value(value)
    , NewPts(outPts)
    , NewPolys(tris)
    , NumPts(0)
    , NumTris(0)
    , NumThreadsUsed(0)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
    , Sequential(seq)
  {
  }

  // Set up the iteration process.
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() method implemented by subclasses (with and without scalar tree)

  // Produce points for non-merged points. This is basically a parallel copy
  // into the final SVTK points array.
  template <typename TP>
  struct ProducePoints
  {
    const std::vector<LocalPtsType*>* LocalPts;
    const std::vector<svtkIdType>* PtOffsets;
    TP* OutPts;
    ProducePoints(const std::vector<LocalPtsType*>* lp, const std::vector<svtkIdType>* o, TP* outPts)
      : LocalPts(lp)
      , PtOffsets(o)
      , OutPts(outPts)
    {
    }
    void operator()(svtkIdType threadId, svtkIdType endThreadId)
    {

      svtkIdType ptOffset;
      LocalPtsType* lPts;
      typename LocalPtsType::iterator pItr, pEnd;
      TP* pts;

      for (; threadId < endThreadId; ++threadId)
      {
        ptOffset = (*this->PtOffsets)[threadId];
        pts = this->OutPts + 3 * ptOffset;
        lPts = (*this->LocalPts)[threadId];
        pEnd = lPts->end();
        for (pItr = lPts->begin(); pItr != pEnd;)
        {
          *pts++ = *pItr++;
        }
      }
    }
  };

  // Functor to build the SVTK triangle list in parallel from the generated
  // points. In the fast path there are three points for every triangle. Many
  // points are typically duplicates but point merging is a significant cost
  // so is ignored in the fast path.
  struct ProduceTriangles
  {
    struct Impl
    {
      template <typename CellStateT>
      void operator()(CellStateT& state, const svtkIdType triBegin, const svtkIdType triEnd,
        const svtkIdType totalTris)
      {
        using ValueType = typename CellStateT::ValueType;
        auto* offsets = state.GetOffsets();
        auto* connectivity = state.GetConnectivity();

        const svtkIdType offsetsBegin = totalTris + triBegin;
        const svtkIdType offsetsEnd = totalTris + triEnd + 1;
        ValueType offset = static_cast<ValueType>(3 * (totalTris + triBegin - 1));
        auto offsetsRange = svtk::DataArrayValueRange<1>(offsets, offsetsBegin, offsetsEnd);
        std::generate(
          offsetsRange.begin(), offsetsRange.end(), [&]() -> ValueType { return offset += 3; });

        const svtkIdType connBegin = 3 * offsetsBegin;
        const svtkIdType connEnd = 3 * (offsetsEnd - 1);
        const ValueType startPtId = static_cast<ValueType>(3 * (totalTris + triBegin));

        auto connRange = svtk::DataArrayValueRange<1>(connectivity, connBegin, connEnd);
        std::iota(connRange.begin(), connRange.end(), startPtId);
      }
    };

    svtkIdType TotalTris;
    svtkCellArray* Tris;
    ProduceTriangles(svtkIdType totalTris, svtkCellArray* tris)
      : TotalTris(totalTris)
      , Tris(tris)
    {
    }
    void operator()(svtkIdType triId, svtkIdType endTriId)
    {
      this->Tris->Visit(Impl{}, triId, endTriId, this->TotalTris);
    }
  };

  // Composite results from each thread
  void Reduce()
  {
    // Count the number of points. For fun keep track of the number of
    // threads used. Also keep track of the thread id so they can
    // be processed in parallel later (copy points in ProducePoints).
    svtkIdType numPts = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    std::vector<LocalPtsType*> localPts;
    std::vector<svtkIdType> localPtOffsets;
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      localPts.push_back(&((*ldItr).LocalPts));
      localPtOffsets.push_back((this->TotalPts + numPts));
      numPts += static_cast<svtkIdType>(((*ldItr).LocalPts.size() / 3)); // x-y-z components
      this->NumThreadsUsed++;
    }

    // (Re)Allocate space for output. Multiple contours require writing into
    // the end of the arrays.
    this->NumPts = numPts;
    this->NumTris = numPts / 3;
    this->NewPts->GetData()->WriteVoidPointer(0, 3 * (this->NumPts + this->TotalPts));
    TOP* pts = static_cast<TOP*>(this->NewPts->GetVoidPointer(0));
    this->NewPolys->ResizeExact(
      this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy points output to SVTK structures. Only point coordinates are
    // copied for now; later we'll define the triangle topology.
    ProducePoints<TOP> producePts(&localPts, &localPtOffsets, pts);
    EXECUTE_SMPFOR(this->Sequential, this->NumThreadsUsed, producePts)

    // Now produce the output triangles (topology) for this contour n parallel
    ProduceTriangles produceTris(this->TotalTris, this->NewPolys);
    EXECUTE_SMPFOR(this->Sequential, this->NumTris, produceTris)
  } // Reduce
};  // ContourCellsBase

// Fast path operator() without scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCells : public ContourCellsBase<TIP, TOP, TS>
{
  ContourCells(TIP* inPts, CellIter* iter, TS* s, double value, svtkPoints* outPts,
    svtkCellArray* tris, svtkIdType totalPts, svtkIdType totalTris, svtkTypeBool seq)
    : ContourCellsBase<TIP, TOP, TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris, seq)
  {
  }

  // Set up the iteration process.
  void Initialize() { this->ContourCellsBase<TIP, TOP, TS>::Initialize(); }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle)
  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lPts = localData.LocalPts;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c = cellIter->Initialize(cellId);
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const TIP* x[MAX_CELL_VERTS];

    for (; cellId < endCellId; ++cellId)
    {
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
        isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
      }
      edges = cellIter->GetCase(isoCase);

      if (*edges > 0)
      {
        numEdges = *edges++;
        for (i = 0; i < cellIter->NumVerts; ++i)
        {
          x[i] = this->InPts + 3 * c[i];
        }

        for (i = 0; i < numEdges; ++i, edges += 2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          lPts.emplace_back(x[v0][0] + t * (x[v1][0] - x[v0][0]));
          lPts.emplace_back(x[v0][1] + t * (x[v1][1] - x[v0][1]));
          lPts.emplace_back(x[v0][2] + t * (x[v1][2] - x[v0][2]));
        }                   // for all edges in this case
      }                     // if contour passes through this cell
      c = cellIter->Next(); // move to the next cell
    }                       // for all cells in this batch
  }

  // Composite results from each thread
  void Reduce() { this->ContourCellsBase<TIP, TOP, TS>::Reduce(); } // Reduce
};                                                                  // ContourCells

// Fast path operator() with a scalar tree
template <typename TIP, typename TOP, typename TS>
struct ContourCellsST : public ContourCellsBase<TIP, TOP, TS>
{
  svtkScalarTree* ScalarTree;
  svtkIdType NumBatches;

  ContourCellsST(TIP* inPts, CellIter* iter, TS* s, double value, svtkScalarTree* st,
    svtkPoints* outPts, svtkCellArray* tris, svtkIdType totalPts, svtkIdType totalTris, svtkTypeBool seq)
    : ContourCellsBase<TIP, TOP, TS>(inPts, iter, s, value, outPts, tris, totalPts, totalTris, seq)
    , ScalarTree(st)
  {
    //    this->ScalarTree->BuildTree();
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }

  // Set up the iteration process.
  void Initialize() { this->ContourCellsBase<TIP, TOP, TS>::Initialize(); }

  // operator() method extracts points from cells (points taken three at a
  // time form a triangle). Uses a scalar tree to accelerate operations.
  void operator()(svtkIdType batchNum, svtkIdType endBatchNum)
  {
    auto& localData = this->LocalData.Local();
    auto& lPts = localData.LocalPts;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c;
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const TIP* x[MAX_CELL_VERTS];
    const svtkIdType* cellIds;
    svtkIdType idx, numCells;

    for (; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
          isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
        }
        edges = cellIter->GetCase(isoCase);

        if (*edges > 0)
        {
          numEdges = *edges++;
          for (i = 0; i < cellIter->NumVerts; ++i)
          {
            x[i] = this->InPts + 3 * c[i];
          }

          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            lPts.emplace_back(x[v0][0] + t * (x[v1][0] - x[v0][0]));
            lPts.emplace_back(x[v0][1] + t * (x[v1][1] - x[v0][1]));
            lPts.emplace_back(x[v0][2] + t * (x[v1][2] - x[v0][2]));
          } // for all edges in this case
        }   // if contour passes through this cell
      }     // for all cells in this batch
    }       // for each batch
  }

  // Composite results from each thread
  void Reduce() { this->ContourCellsBase<TIP, TOP, TS>::Reduce(); } // Reduce
};                                                                  // ContourCellsST

// Dispatch method for Fast path processing. Handles template dispatching etc.
template <typename TS>
void ProcessFastPath(svtkIdType numCells, svtkPoints* inPts, CellIter* cellIter, TS* s,
  double isoValue, svtkScalarTree* st, svtkPoints* outPts, svtkCellArray* tris, svtkTypeBool seq,
  int& numThreads, svtkIdType totalPts, svtkIdType totalTris)
{
  double val = static_cast<double>(isoValue);
  int inPtsType = inPts->GetDataType();
  void* inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_FLOAT)
  {
    if (st != nullptr)
    {
      ContourCellsST<float, float, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<float, float, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else if (inPtsType == SVTK_DOUBLE && outPtsType == SVTK_DOUBLE)
  {
    if (st != nullptr)
    {
      ContourCellsST<double, double, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<double, double, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_DOUBLE)
  {
    if (st != nullptr)
    {
      ContourCellsST<float, double, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<float, double, TS> contour(
        (float*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
  else // if ( inPtsType == SVTK_DOUBLE && outPtsType == SVTK_FLOAT )
  {
    if (st != nullptr)
    {
      ContourCellsST<double, float, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, st, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, contour.NumBatches, contour, numThreads);
    }
    else
    {
      ContourCells<double, float, TS> contour(
        (double*)inPtsPtr, cellIter, (TS*)s, val, outPts, tris, totalPts, totalTris, seq);
      EXECUTE_REDUCED_SMPFOR(seq, numCells, contour, numThreads);
    }
  }
};

//========================= GENERAL PATH (POINT MERGING) =======================
// Use svtkStaticEdgeLocatorTemplate for edge-based point merging. Processing is
// available with and without a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesBase
{
  typedef std::vector<EdgeTuple<IDType, float> > EdgeVectorType;
  typedef std::vector<MergeTuple<IDType, float> > MergeVectorType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    EdgeVectorType LocalEdges;
    CellIter LocalCellIter;

    LocalDataType() { this->LocalEdges.reserve(2048); }
  };

  CellIter* Iter;
  const TS* Scalars;
  double Value;
  MergeTuple<IDType, float>* Edges;
  svtkCellArray* Tris;
  svtkIdType NumTris;
  int NumThreadsUsed;
  svtkIdType TotalTris; // the total triangles thus far (support multiple contours)
  svtkTypeBool Sequential;

  // Keep track of generated points and triangles on a per thread basis
  svtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractEdgesBase(
    CellIter* c, TS* s, double value, svtkCellArray* tris, svtkIdType totalTris, svtkTypeBool seq)
    : Iter(c)
    , Scalars(s)
    , Value(value)
    , Edges(nullptr)
    , Tris(tris)
    , NumTris(0)
    , NumThreadsUsed(0)
    , TotalTris(totalTris)
    , Sequential(seq)
  {
  }

  // Set up the iteration process
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() provided by subclass

  // Produce edges for merged points. This is basically a parallel composition
  // into the final edges array.
  template <typename IDT>
  struct ProduceEdges
  {
    const std::vector<EdgeVectorType*>* LocalEdges;
    const std::vector<svtkIdType>* TriOffsets;
    MergeTuple<IDT, float>* OutEdges;
    ProduceEdges(const std::vector<EdgeVectorType*>* le, const std::vector<svtkIdType>* o,
      MergeTuple<IDT, float>* outEdges)
      : LocalEdges(le)
      , TriOffsets(o)
      , OutEdges(outEdges)
    {
    }
    void operator()(svtkIdType threadId, svtkIdType endThreadId)
    {
      svtkIdType triOffset, edgeNum;
      const EdgeVectorType* lEdges;
      MergeTuple<IDT, float>* edges;

      for (; threadId < endThreadId; ++threadId)
      {
        triOffset = (*this->TriOffsets)[threadId];
        edgeNum = 3 * triOffset;
        edges = this->OutEdges + edgeNum;
        lEdges = (*this->LocalEdges)[threadId];
        auto eEnd = lEdges->end();
        for (auto eItr = lEdges->begin(); eItr != eEnd; ++eItr)
        {
          edges->V0 = eItr->V0;
          edges->V1 = eItr->V1;
          edges->T = eItr->T;
          edges->EId = edgeNum;
          edges++;
          edgeNum++;
        }
      } // for all threads
    }
  };

  // Composite local thread data
  void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    svtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    std::vector<EdgeVectorType*> localEdges;
    std::vector<svtkIdType> localTriOffsets;
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      localEdges.push_back(&((*ldItr).LocalEdges));
      localTriOffsets.push_back(numTris);
      numTris +=
        static_cast<svtkIdType>(((*ldItr).LocalEdges.size() / 3)); // three edges per triangle
      this->NumThreadsUsed++;
    }

    // Allocate space for SVTK triangle output. Take into account previous
    // contours.
    this->NumTris = numTris;
    this->Tris->ResizeExact(this->NumTris + this->TotalTris, 3 * (this->NumTris + this->TotalTris));

    // Copy local edges to composited edge array.
    this->Edges = new MergeTuple<IDType, float>[3 * this->NumTris]; // three edges per triangle
    ProduceEdges<IDType> produceEdges(&localEdges, &localTriOffsets, this->Edges);
    EXECUTE_SMPFOR(this->Sequential, this->NumThreadsUsed, produceEdges);
    // EdgeVectorType emptyVector;
    //(*ldItr).LocalEdges.swap(emptyVector); //frees memory

  } // Reduce
};  // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without scalar tree).
template <typename IDType, typename TS>
struct ExtractEdges : public ExtractEdgesBase<IDType, TS>
{
  ExtractEdges(
    CellIter* c, TS* s, double value, svtkCellArray* tris, svtkIdType totalTris, svtkTypeBool seq)
    : ExtractEdgesBase<IDType, TS>(c, s, value, tris, totalTris, seq)
  {
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TS>::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(svtkIdType cellId, svtkIdType endCellId)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c = cellIter->Initialize(cellId); // connectivity array
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;

    for (; cellId < endCellId; ++cellId)
    {
      // Compute case by repeated masking of scalar value
      for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
      {
        s[i] = static_cast<double>(*(this->Scalars + c[i]));
        isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
      }
      edges = cellIter->GetCase(isoCase);

      if (*edges > 0)
      {
        numEdges = *edges++;
        for (i = 0; i < numEdges; ++i, edges += 2)
        {
          v0 = edges[0];
          v1 = edges[1];
          deltaScalar = s[v1] - s[v0];
          t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
          t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
          lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
        }                                       // for all edges in this case
      }                                         // if contour passes through this cell
      c = cellIter->Next();                     // move to the next cell
    }                                           // for all cells in this batch
  }

  // Composite local thread data
  void Reduce() { this->ExtractEdgesBase<IDType, TS>::Reduce(); } // Reduce
};                                                                // ExtractEdges

// Generate edges using a scalar tree.
template <typename IDType, typename TS>
struct ExtractEdgesST : public ExtractEdgesBase<IDType, TS>
{
  svtkScalarTree* ScalarTree;
  svtkIdType NumBatches;

  ExtractEdgesST(CellIter* c, TS* s, double value, svtkScalarTree* st, svtkCellArray* tris,
    svtkIdType totalTris, svtkTypeBool seq)
    : ExtractEdgesBase<IDType, TS>(c, s, value, tris, totalTris, seq)
    , ScalarTree(st)
  {
    this->NumBatches = this->ScalarTree->GetNumberOfCellBatches(value);
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TS>::Initialize(); }

  // operator() method extracts edges from cells (edges taken three at a
  // time form a triangle)
  void operator()(svtkIdType batchNum, svtkIdType endBatchNum)
  {
    auto& localData = this->LocalData.Local();
    auto& lEdges = localData.LocalEdges;
    CellIter* cellIter = &localData.LocalCellIter;
    const svtkIdType* c;
    unsigned short isoCase, numEdges, i;
    const unsigned short* edges;
    double s[MAX_CELL_VERTS], value = this->Value, deltaScalar;
    float t;
    unsigned char v0, v1;
    const svtkIdType* cellIds;
    svtkIdType idx, numCells;

    for (; batchNum < endBatchNum; ++batchNum)
    {
      cellIds = this->ScalarTree->GetCellBatch(batchNum, numCells);
      for (idx = 0; idx < numCells; ++idx)
      {
        c = cellIter->GetCellIds(cellIds[idx]);
        // Compute case by repeated masking of scalar value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          s[i] = static_cast<double>(*(this->Scalars + c[i]));
          isoCase |= (s[i] >= value ? BaseCell::Mask[i] : 0);
        }
        edges = cellIter->GetCase(isoCase);

        if (*edges > 0)
        {
          numEdges = *edges++;
          for (i = 0; i < numEdges; ++i, edges += 2)
          {
            v0 = edges[0];
            v1 = edges[1];
            deltaScalar = s[v1] - s[v0];
            t = (deltaScalar == 0.0 ? 0.0 : (value - s[v0]) / deltaScalar);
            t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
            lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
          }                                       // for all edges in this case
        }                                         // if contour passes through this cell
      }                                           // for all cells in this batch
    }                                             // for all batches
  }

  // Composite local thread data
  void Reduce() { this->ExtractEdgesBase<IDType, TS>::Reduce(); } // Reduce

}; // ExtractEdgesST

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProduceMergedTriangles
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  svtkIdType NumTris;
  svtkCellArray* Tris;
  svtkIdType TotalPts;
  svtkIdType TotalTris;
  int NumThreadsUsed; // placeholder

  ProduceMergedTriangles(const MergeTupleType* merge, const IDType* offsets, svtkIdType numTris,
    svtkCellArray* tris, svtkIdType totalPts, svtkIdType totalTris)
    : MergeArray(merge)
    , Offsets(offsets)
    , NumTris(numTris)
    , Tris(tris)
    , TotalPts(totalPts)
    , TotalTris(totalTris)
    , NumThreadsUsed(1)
  {
  }

  void Initialize()
  {
    ; // without this method Reduce() is not called
  }

  struct Impl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, svtkIdType ptId, const svtkIdType endPtId,
      const svtkIdType ptOffset, const svtkIdType connOffset, const IDType* offsets,
      const MergeTupleType* mergeArray)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* conn = state.GetConnectivity();

      for (; ptId < endPtId; ++ptId)
      {
        const IDType numPtsInGroup = offsets[ptId + 1] - offsets[ptId];
        for (IDType i = 0; i < numPtsInGroup; ++i)
        {
          const IDType connIdx = mergeArray[offsets[ptId] + i].EId + connOffset;
          conn->SetValue(connIdx, static_cast<ValueType>(ptId + ptOffset));
        } // for this group of coincident edges
      }   // for all merged points
    }
  };

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    this->Tris->Visit(
      Impl{}, ptId, endPtId, this->TotalPts, 3 * this->TotalTris, this->Offsets, this->MergeArray);
  }

  struct ReduceImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, const svtkIdType totalTris, const svtkIdType nTris)
    {
      using ValueType = typename CellStateT::ValueType;

      auto offsets =
        svtk::DataArrayValueRange<1>(state.GetOffsets(), totalTris, totalTris + nTris + 1);
      ValueType offset = 3 * (totalTris - 1); // +=3 on first access
      std::generate(offsets.begin(), offsets.end(), [&]() -> ValueType { return offset += 3; });
    }
  };

  // Update the triangle connectivity (numPts for each triangle. This could
  // be done in parallel but it's probably not faster.
  void Reduce() { this->Tris->Visit(ReduceImpl{}, this->TotalTris, this->NumTris); }
};

// This method generates the output isosurface points. One point per
// merged edge is generated.
template <typename TIP, typename TOP, typename IDType>
struct ProduceMergedPoints
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  const TIP* InPts;
  TOP* OutPts;

  ProduceMergedPoints(
    const MergeTupleType* merge, const IDType* offsets, TIP* inPts, TOP* outPts, svtkIdType totalPts)
    : MergeArray(merge)
    , Offsets(offsets)
    , InPts(inPts)
  {
    this->OutPts = outPts + 3 * totalPts;
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const MergeTupleType* mergeTuple;
    IDType v0, v1;
    const TIP *x0, *x1, *inPts = this->InPts;
    TOP *x, *outPts = this->OutPts;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->MergeArray + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      x0 = inPts + 3 * v0;
      x1 = inPts + 3 * v1;
      x = outPts + 3 * ptId;
      x[0] = x0[0] + t * (x1[0] - x0[0]);
      x[1] = x0[1] + t * (x1[1] - x0[1]);
      x[2] = x0[2] + t * (x1[2] - x0[2]);
    }
  }
};

// If requested, interpolate point data attributes. The merge tuple contains an
// interpolation value t for the merged edge.
template <typename TIds>
struct ProduceAttributes
{
  const MergeTuple<TIds, float>* Edges; // all edges, sorted into groups of merged edges
  const TIds* Offsets;                  // refer to single, unique, merged edge
  ArrayList* Arrays;                    // carry list of attributes to interpolate
  svtkIdType TotalPts;                   // total points / multiple contours computed previously

  ProduceAttributes(
    const MergeTuple<TIds, float>* mt, const TIds* offsets, ArrayList* arrays, svtkIdType totalPts)
    : Edges(mt)
    , Offsets(offsets)
    , Arrays(arrays)
    , TotalPts(totalPts)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const MergeTuple<TIds, float>* mergeTuple;
    TIds v0, v1;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->Edges + this->Offsets[ptId];
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      this->Arrays->InterpolateEdge(v0, v1, t, ptId + this->TotalPts);
    }
  }
};

// Make the source code a little more readable
#define EXTRACT_MERGED(SVTK_type, _type)                                                            \
  case SVTK_type:                                                                                   \
  {                                                                                                \
    if (st == nullptr)                                                                             \
    {                                                                                              \
      ExtractEdges<TIds, _type> extractEdges(                                                      \
        cellIter, (_type*)s, isoValue, newPolys, totalTris, seqProcessing);                        \
      EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);                   \
      numTris = extractEdges.NumTris;                                                              \
      mergeEdges = extractEdges.Edges;                                                             \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ExtractEdgesST<TIds, _type> extractEdges(                                                    \
        cellIter, (_type*)s, isoValue, st, newPolys, totalTris, seqProcessing);                    \
      EXECUTE_REDUCED_SMPFOR(seqProcessing, extractEdges.NumBatches, extractEdges, numThreads);    \
      numTris = extractEdges.NumTris;                                                              \
      mergeEdges = extractEdges.Edges;                                                             \
    }                                                                                              \
  }                                                                                                \
  break;

// Wrapper to handle multiple template types for merged processing
template <typename TIds>
int ProcessMerged(svtkIdType numCells, svtkPoints* inPts, CellIter* cellIter, int sType, void* s,
  double isoValue, svtkPoints* outPts, svtkCellArray* newPolys, svtkTypeBool intAttr,
  svtkDataArray* inScalars, svtkPointData* inPD, svtkPointData* outPD, ArrayList* arrays,
  svtkScalarTree* st, svtkTypeBool seqProcessing, int& numThreads, svtkIdType totalPts,
  svtkIdType totalTris)
{
  // Extract edges that the contour intersects. Templated on type of scalars.
  // List below the explicit choice of scalars that can be processed.
  svtkIdType numTris = 0;
  MergeTuple<TIds, float>* mergeEdges = nullptr; // may need reference counting
  switch (sType) // process these scalar types, others could easily be added
  {
    EXTRACT_MERGED(SVTK_UNSIGNED_INT, unsigned int);
    EXTRACT_MERGED(SVTK_INT, int);
    EXTRACT_MERGED(SVTK_FLOAT, float);
    EXTRACT_MERGED(SVTK_DOUBLE, double);
    default:
      svtkGenericWarningMacro(<< "Scalar type not supported");
      return 0;
  };
  int nt = numThreads;

  // Make sure data was produced
  if (numTris <= 0)
  {
    delete[] mergeEdges;
    return 1;
  }

  // Merge coincident edges. The Offsets refer to the single unique edge
  // from the sorted group of duplicate edges.
  svtkIdType numPts;
  svtkStaticEdgeLocatorTemplate<TIds, float> loc;
  const TIds* offsets = loc.MergeEdges(3 * numTris, mergeEdges, numPts);

  // Generate triangles.
  ProduceMergedTriangles<TIds> produceTris(
    mergeEdges, offsets, numTris, newPolys, totalPts, totalTris);
  EXECUTE_REDUCED_SMPFOR(seqProcessing, numPts, produceTris, numThreads);
  numThreads = nt;

  // Generate points (one per unique edge)
  outPts->GetData()->WriteVoidPointer(0, 3 * (numPts + totalPts));
  int inPtsType = inPts->GetDataType();
  void* inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  void* outPtsPtr = outPts->GetVoidPointer(0);

  // Only handle combinations of real types
  if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_FLOAT)
  {
    ProduceMergedPoints<float, float, TIds> producePts(
      mergeEdges, offsets, (float*)inPtsPtr, (float*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else if (inPtsType == SVTK_DOUBLE && outPtsType == SVTK_DOUBLE)
  {
    ProduceMergedPoints<double, double, TIds> producePts(
      mergeEdges, offsets, (double*)inPtsPtr, (double*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_DOUBLE)
  {
    ProduceMergedPoints<float, double, TIds> producePts(
      mergeEdges, offsets, (float*)inPtsPtr, (double*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }
  else // if ( inPtsType == SVTK_DOUBLE && outPtsType == SVTK_FLOAT )
  {
    ProduceMergedPoints<double, float, TIds> producePts(
      mergeEdges, offsets, (double*)inPtsPtr, (float*)outPtsPtr, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
  }

  // Now process point data attributes if requested
  if (intAttr)
  {
    if (totalPts <= 0) // first contour value generating output
    {
      outPD->InterpolateAllocate(inPD, numPts);
      outPD->RemoveArray(inScalars->GetName());
      arrays->ExcludeArray(inScalars);
      arrays->AddArrays(numPts, inPD, outPD);
    }
    else
    {
      arrays->Realloc(totalPts + numPts);
    }
    ProduceAttributes<TIds> interpolate(mergeEdges, offsets, arrays, totalPts);
    EXECUTE_SMPFOR(seqProcessing, numPts, interpolate);
  }

  // Clean up
  delete[] mergeEdges;
  return 1;
}
#undef EXTRACT_MERGED

// Functor for computing cell normals. Could easily be templated on output
// point type but we are trying to control object size.
struct ComputeCellNormals
{
  svtkPoints* Points;
  svtkCellArray* Tris;
  float* CellNormals;

  ComputeCellNormals(svtkPoints* pts, svtkCellArray* tris, float* cellNormals)
    : Points(pts)
    , Tris(tris)
    , CellNormals(cellNormals)
  {
  }

  void operator()(svtkIdType triId, svtkIdType endTriId)
  {
    auto cellIt = svtk::TakeSmartPointer(this->Tris->NewIterator());

    float* n = this->CellNormals + 3 * triId;
    double nd[3];

    svtkIdType unused = 3;
    const svtkIdType* tri = nullptr;

    for (cellIt->GoToCell(triId); cellIt->GetCurrentCellId() < endTriId; cellIt->GoToNextCell())
    {
      cellIt->GetCurrentCell(unused, tri);
      svtkTriangle::ComputeNormal(this->Points, 3, tri, nd);
      *n++ = nd[0];
      *n++ = nd[1];
      *n++ = nd[2];
    }
  }
};

// Generate normals on output triangles
svtkFloatArray* GenerateTriNormals(svtkTypeBool seqProcessing, svtkPoints* pts, svtkCellArray* tris)
{
  svtkIdType numTris = tris->GetNumberOfCells();

  svtkFloatArray* cellNormals = svtkFloatArray::New();
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numTris);
  float* n = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Execute functor over all triangles
  ComputeCellNormals computeNormals(pts, tris, n);
  EXECUTE_SMPFOR(seqProcessing, numTris, computeNormals);

  return cellNormals;
}

// Functor for averaging normals at each merged point.
template <typename TId>
struct AverageNormals
{
  svtkStaticCellLinksTemplate<TId>* Links;
  const float* CellNormals;
  float* PointNormals;

  AverageNormals(svtkStaticCellLinksTemplate<TId>* links, float* cellNormals, float* ptNormals)
    : Links(links)
    , CellNormals(cellNormals)
    , PointNormals(ptNormals)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    TId i, numTris;
    const TId* tris;
    const float* nc;
    float* n = this->PointNormals + 3 * ptId;

    for (; ptId < endPtId; ++ptId, n += 3)
    {
      numTris = this->Links->GetNumberOfCells(ptId);
      tris = this->Links->GetCells(ptId);
      n[0] = n[1] = n[2] = 0.0;
      for (i = 0; i < numTris; ++i)
      {
        nc = this->CellNormals + 3 * tris[i];
        n[0] += nc[0];
        n[1] += nc[1];
        n[2] += nc[2];
      }
      svtkMath::Normalize(n);
    }
  }
};

// Generate normals on merged points. Average cell normals at each point.
template <typename TId>
void GeneratePointNormals(svtkTypeBool seqProcessing, svtkPoints* pts, svtkCellArray* tris,
  svtkFloatArray* cellNormals, svtkPointData* pd)
{
  svtkIdType numPts = pts->GetNumberOfPoints();

  svtkFloatArray* ptNormals = svtkFloatArray::New();
  ptNormals->SetName("Normals");
  ptNormals->SetNumberOfComponents(3);
  ptNormals->SetNumberOfTuples(numPts);
  float* ptN = static_cast<float*>(ptNormals->GetVoidPointer(0));

  // Grab the computed triangle normals
  float* triN = static_cast<float*>(cellNormals->GetVoidPointer(0));

  // Build cell links
  svtkPolyData* dummy = svtkPolyData::New();
  dummy->SetPoints(pts);
  dummy->SetPolys(tris);
  svtkStaticCellLinksTemplate<TId> links;
  links.BuildLinks(dummy);

  // Process all points, averaging normals
  AverageNormals<TId> average(&links, triN, ptN);
  EXECUTE_SMPFOR(seqProcessing, numPts, average);

  // Clean up and get out
  dummy->Delete();
  pd->SetNormals(ptNormals);
  cellNormals->Delete();
  ptNormals->Delete();
};

} // anonymous namespace

// Map scalar trees to input datasets. Necessary due to potential composite
// data set input types, where each piece may have a different scalar tree.
struct svtkScalarTreeMap : public std::map<svtkUnstructuredGrid*, svtkScalarTree*>
{
};

//-----------------------------------------------------------------------------
// Construct an instance of the class.
svtkContour3DLinearGrid::svtkContour3DLinearGrid()
{
  this->ContourValues = svtkContourValues::New();

  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // By default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  this->MergePoints = false;
  this->InterpolateAttributes = false;
  this->ComputeNormals = false;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;

  this->UseScalarTree = 0;
  this->ScalarTree = nullptr;
  this->ScalarTreeMap = new svtkScalarTreeMap;
}

//-----------------------------------------------------------------------------
svtkContour3DLinearGrid::~svtkContour3DLinearGrid()
{
  this->ContourValues->Delete();

  // Need to free scalar trees associated with each dataset. There is a
  // special case where the stree cannot be deleted because is has been
  // specified by the user.
  svtkScalarTree* stree;
  svtkScalarTreeMap::iterator iter;
  for (iter = this->ScalarTreeMap->begin(); iter != this->ScalarTreeMap->end(); ++iter)
  {
    stree = iter->second;
    if (stree != nullptr && stree != this->ScalarTree)
    {
      stree->Delete();
    }
  }
  delete this->ScalarTreeMap;

  if (this->ScalarTree)
  {
    this->ScalarTree->Delete();
    this->ScalarTree = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
svtkMTimeType svtkContour3DLinearGrid::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

// Make code more readable
#define EXTRACT_FAST_PATH(SVTK_SCALAR_type, _type)                                                  \
  case SVTK_SCALAR_type:                                                                            \
    ProcessFastPath<_type>(numCells, inPts, cellIter, (_type*)sPtr, value, stree, outPts,          \
      newPolys, this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris);       \
    break;

//-----------------------------------------------------------------------------
// Specialized contouring filter to handle unstructured grids with 3D linear
// cells (tetrahedras, hexes, wedges, pyradmids, voxels).
//
void svtkContour3DLinearGrid::ProcessPiece(
  svtkUnstructuredGrid* input, svtkDataArray* inScalars, svtkPolyData* output)
{

  // Make sure there is data to process
  svtkCellArray* cells = input->GetCells();
  svtkIdType numPts, numCells;
  if (cells == nullptr || (numCells = cells->GetNumberOfCells()) < 1)
  {
    svtkDebugMacro(<< "No data in this piece");
    return;
  }

  // Get the contour values.
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double value, *values = this->ContourValues->GetValues();

  // Setup scalar processing
  int sType = inScalars->GetDataType();
  void* sPtr = inScalars->GetVoidPointer(0);

  // Check the input point type. Only real types are supported.
  svtkPoints* inPts = input->GetPoints();
  numPts = inPts->GetNumberOfPoints();
  int inPtsType = inPts->GetDataType();
  if ((inPtsType != SVTK_FLOAT && inPtsType != SVTK_DOUBLE))
  {
    svtkLog(ERROR, "Input point type not supported");
    return;
  }
  // Create the output points. Only real types are supported.
  svtkPoints* outPts = svtkPoints::New();
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    outPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    outPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    outPts->SetDataType(SVTK_DOUBLE);
  }

  // Compute the scalar array range difference between min and max is 0.0, do not use
  // a scalar tree (no contour will be generated anyway).
  double scalarRange[2];
  inScalars->GetRange(scalarRange);
  double rangeDiff = scalarRange[1] - scalarRange[0];

  // If a scalar tree is requested, retrieve previous or if not found,
  // create a default or clone the factory.
  svtkScalarTree* stree = nullptr;
  if (this->UseScalarTree && rangeDiff > 0.0)
  {
    svtkScalarTreeMap::iterator mapIter = this->ScalarTreeMap->find(input);
    if (mapIter == this->ScalarTreeMap->end())
    {
      if (this->ScalarTree)
      {
        stree = this->ScalarTree->NewInstance();
        stree->ShallowCopy(this->ScalarTree);
      }
      else
      {
        stree = svtkSpanSpace::New(); // default type if not provided
      }
      this->ScalarTreeMap->insert(std::make_pair(input, stree));
    }
    else
    {
      stree = mapIter->second;
    }
    // These will not cause a Modified() if the values haven't changed.
    stree->SetDataSet(input);
    stree->SetScalars(inScalars);
  }

  // Output triangles go here.
  svtkCellArray* newPolys = svtkCellArray::New();

  // Process all contour values
  svtkIdType totalPts = 0;
  svtkIdType totalTris = 0;

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Now produce the output: fast path or general path
  int mergePoints = this->MergePoints | this->ComputeNormals | this->InterpolateAttributes;
  if (!mergePoints)
  { // fast path
    // Generate all of the points at once (for multiple contours) and then produce the triangles.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      switch (sType) // process these scalar types, others could easily be added
      {
        EXTRACT_FAST_PATH(SVTK_UNSIGNED_INT, unsigned int);
        EXTRACT_FAST_PATH(SVTK_INT, int);
        EXTRACT_FAST_PATH(SVTK_FLOAT, float);
        EXTRACT_FAST_PATH(SVTK_DOUBLE, double);
        default:
          svtkGenericWarningMacro(<< "Scalar type not supported");
          return;
      };

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    } // for all contours
  }

  else // Need to merge points, and possibly perform attribute interpolation
       // and generate normals. Hence use the slower path.
  {
    svtkPointData* inPD = input->GetPointData();
    svtkPointData* outPD = output->GetPointData();
    ArrayList arrays;

    // Determine the size/type of point and cell ids needed to index points
    // and cells. Using smaller ids results in a greatly reduced memory footprint
    // and faster processing.
    this->LargeIds = (numPts >= SVTK_INT_MAX || numCells >= SVTK_INT_MAX ? true : false);

    // Generate all of the merged points and triangles at once (for multiple
    // contours) and then produce the normals if requested.
    for (int vidx = 0; vidx < numContours; vidx++)
    {
      value = values[vidx];
      if (this->LargeIds == false)
      {
        if (!ProcessMerged<int>(numCells, inPts, cellIter, sType, sPtr, value, outPts, newPolys,
              this->InterpolateAttributes, inScalars, inPD, outPD, &arrays, stree,
              this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris))
        {
          return;
        }
      }
      else
      {
        if (!ProcessMerged<svtkIdType>(numCells, inPts, cellIter, sType, sPtr, value, outPts,
              newPolys, this->InterpolateAttributes, inScalars, inPD, outPD, &arrays, stree,
              this->SequentialProcessing, this->NumberOfThreadsUsed, totalPts, totalTris))
        {
          return;
        }
      }

      // Multiple contour values require accumulating points & triangles
      totalPts = outPts->GetNumberOfPoints();
      totalTris = newPolys->GetNumberOfCells();
    } // for all contour values

    // If requested, compute normals. Basically triangle normals are averaged
    // on each merged point. Requires building static CellLinks so it is a
    // relatively expensive operation. (This block of code is separate to
    // control .obj object bloat.)
    if (this->ComputeNormals)
    {
      svtkFloatArray* triNormals = GenerateTriNormals(this->SequentialProcessing, outPts, newPolys);
      if (this->LargeIds)
      {
        GeneratePointNormals<svtkIdType>(
          this->SequentialProcessing, outPts, newPolys, triNormals, outPD);
      }
      else
      {
        GeneratePointNormals<int>(this->SequentialProcessing, outPts, newPolys, triNormals, outPD);
      }
    }
  } // slower path requires point merging

  // Report the results of execution
  svtkDebugMacro(<< "Created: " << outPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  // Clean up
  delete cellIter;
  output->SetPoints(outPts);
  outPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
}

//----------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int svtkContour3DLinearGrid::RequestDataObject(
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
    if (svtkPolyData::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkPolyData::New();
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

  svtkErrorMacro("Not sure what type of output to create!");
  return 0;
}

//-----------------------------------------------------------------------------
// RequestData checks the input, manages composite data, and handles the
// (optional) scalar tree. For each input svtkUnstructuredGrid, it produces an
// output svtkPolyData piece by performing contouring on the input dataset.
//
int svtkContour3DLinearGrid::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input and output
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkUnstructuredGrid* inputGrid =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputPolyData =
    svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkCompositeDataSet* inputCDS =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkMultiBlockDataSet* outputMBDS =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Make sure we have valid input and output of some form
  if ((inputGrid == nullptr || outputPolyData == nullptr) &&
    (inputCDS == nullptr || outputMBDS == nullptr))
  {
    return 0;
  }

  // Get the contour values.
  int numContours = this->ContourValues->GetNumberOfContours();
  if (numContours < 1)
  {
    svtkLog(TRACE, "No contour values defined");
    return 1;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output svtkPolyData.
  svtkDataArray* inScalars;
  if (inputGrid)
  {
    // Get the scalars to process
    inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!inScalars)
    {
      svtkLog(TRACE, "No scalars available");
      return 1;
    }

    double scalarRange[2];
    inScalars->GetRange(scalarRange);
    double rangeDiff = scalarRange[1] - scalarRange[0];

    // Use provided scalar tree if not a composite data set input and scalar array range
    // difference between min and max is non-zero.
    if (this->UseScalarTree && this->ScalarTree && rangeDiff > 0.0)
    {
      this->ScalarTreeMap->insert(std::make_pair(inputGrid, this->ScalarTree));
    }
    this->ProcessPiece(inputGrid, inScalars, outputPolyData);
  }

  // Otherwise it is an input composite data set and each unstructured grid
  // contained in it is processed, producing a svtkPolyData that is added to
  // the output multiblock dataset.
  else
  {
    svtkUnstructuredGrid* grid;
    svtkPolyData* polydata;
    outputMBDS->CopyStructure(inputCDS);
    svtkSmartPointer<svtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCDS->NewIterator());
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      auto ds = inIter->GetCurrentDataObject();
      if ((grid = svtkUnstructuredGrid::SafeDownCast(ds)))
      {
        int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
        inScalars = this->GetInputArrayToProcess(0, grid, association);
        if (!inScalars)
        {
          svtkLog(TRACE, "No scalars available");
          continue;
        }
        polydata = svtkPolyData::New();
        this->ProcessPiece(grid, inScalars, polydata);
        outputMBDS->SetDataSet(inIter, polydata);
        polydata->Delete();
      }
      else
      {
        svtkDebugMacro(<< "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkContour3DLinearGrid::SetOutputPointsPrecision(int precision)
{
  if (this->OutputPointsPrecision != precision)
  {
    this->OutputPointsPrecision = precision;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int svtkContour3DLinearGrid::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
bool svtkContour3DLinearGrid::CanFullyProcessDataObject(
  svtkDataObject* object, const char* scalarArrayName)
{
  auto ug = svtkUnstructuredGrid::SafeDownCast(object);
  auto cd = svtkCompositeDataSet::SafeDownCast(object);

  if (ug)
  {
    svtkDataArray* array = ug->GetPointData()->GetArray(scalarArrayName);
    if (!array)
    {
      svtkLog(INFO, "Scalar array is null");
      return true;
    }

    int aType = array->GetDataType();
    if (aType != SVTK_UNSIGNED_INT && aType != SVTK_INT && aType != SVTK_FLOAT && aType != SVTK_DOUBLE)
    {
      svtkLog(INFO, "Invalid scalar array type");
      return false;
    }

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
      if (!CanFullyProcessDataObject(leafDS, scalarArrayName))
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
int svtkContour3DLinearGrid::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkContour3DLinearGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Merge Points: " << (this->MergePoints ? "true\n" : "false\n");
  os << indent
     << "Interpolate Attributes: " << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "true\n" : "false\n");

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: " << (this->LargeIds ? "true\n" : "false\n");

  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");
  if (this->ScalarTree)
  {
    os << indent << "Scalar Tree: " << this->ScalarTree << "\n";
  }
  else
  {
    os << indent << "Scalar Tree: (none)\n";
  }
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
#undef EXTRACT_MERGED
#undef EXTRACT_FAST_PATH
