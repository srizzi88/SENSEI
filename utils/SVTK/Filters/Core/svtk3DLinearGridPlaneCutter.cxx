/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DLinearGridPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtk3DLinearGridPlaneCutter.h"

#include "svtk3DLinearGridInternal.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArrayRange.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkHexahedron.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPyramid.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
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
#include <numeric>

svtkStandardNewMacro(svtk3DLinearGridPlaneCutter);
svtkCxxSetObjectMacro(svtk3DLinearGridPlaneCutter, Plane, svtkPlane);

//-----------------------------------------------------------------------------
// Classes to support threaded execution. Note that there is only one
// strategy at this time: a path that pre-computes plane function values and
// uses these to cull non-intersected cells. Sphere trees may be supported in
// the future.

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

//========================= Quick plane cut culling ===========================
// Compute an array that classifies each point with respect to the current
// plane (i.e. above the plane(=2), below the plane(=1), on the plane(=0)).
// InOutArray is allocated here and should be deleted by the invoking
// code. InOutArray is an unsigned char array to simplify bit fiddling later
// on (i.e., PlaneIntersects() method).
//
// The reason we compute this unsigned char array as compared to an array of
// function values is to reduce the amount of memory used, and the written to
// memory, since these are significant costs for large data.

// Templated for explicit point representations of real type
template <typename TP>
struct ClassifyPoints;

struct Classify
{
  unsigned char* InOutArray;
  double Origin[3];
  double Normal[3];

  Classify(svtkPoints* pts, svtkPlane* plane)
  {
    this->InOutArray = new unsigned char[pts->GetNumberOfPoints()];
    plane->GetOrigin(this->Origin);
    plane->GetNormal(this->Normal);
  }

  // Check if a list of points intersects the plane
  static bool PlaneIntersects(const unsigned char* inout, svtkIdType npts, const svtkIdType* pts)
  {
    unsigned char onOneSideOfPlane = inout[pts[0]];
    for (svtkIdType i = 1; onOneSideOfPlane && i < npts; ++i)
    {
      onOneSideOfPlane &= inout[pts[i]];
    }
    return (!onOneSideOfPlane);
  }
};

template <typename TP>
struct ClassifyPoints : public Classify
{
  TP* Points;

  ClassifyPoints(svtkPoints* pts, svtkPlane* plane)
    : Classify(pts, plane)
  {
    this->Points = static_cast<TP*>(pts->GetVoidPointer(0));
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

//========================= Compute edge intersections ========================
// Use svtkStaticEdgeLocatorTemplate for edge-based point merging.
template <typename IDType, typename TIP>
struct ExtractEdgesBase
{
  typedef std::vector<EdgeTuple<IDType, float> > EdgeVectorType;

  // Track local data on a per-thread basis. In the Reduce() method this
  // information will be used to composite the data from each thread.
  struct LocalDataType
  {
    EdgeVectorType LocalEdges;
    CellIter LocalCellIter;

    LocalDataType() { this->LocalEdges.reserve(2048); }
  };

  const TIP* InPts;
  CellIter* Iter;
  MergeTuple<IDType, float>* Edges;
  svtkCellArray* Tris;
  svtkIdType NumTris;
  int NumThreadsUsed;
  double Origin[3];
  double Normal[3];

  // Keep track of generated points and triangles on a per thread basis
  svtkSMPThreadLocal<LocalDataType> LocalData;

  ExtractEdgesBase(TIP* inPts, CellIter* c, svtkPlane* plane, svtkCellArray* tris)
    : InPts(inPts)
    , Iter(c)
    , Edges(nullptr)
    , Tris(tris)
    , NumTris(0)
    , NumThreadsUsed(0)
  {
    plane->GetNormal(this->Normal);
    plane->GetOrigin(this->Origin);
  }

  // Set up the iteration process
  void Initialize()
  {
    auto& localData = this->LocalData.Local();
    localData.LocalCellIter = *(this->Iter);
  }

  // operator() provided by subclass

  // Composite local thread data
  void Reduce()
  {
    // Count the number of triangles, and number of threads used.
    svtkIdType numTris = 0;
    this->NumThreadsUsed = 0;
    auto ldEnd = this->LocalData.end();
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      numTris +=
        static_cast<svtkIdType>(((*ldItr).LocalEdges.size() / 3)); // three edges per triangle
      this->NumThreadsUsed++;
    }

    // Allocate space for SVTK triangle output.
    this->NumTris = numTris;
    this->Tris->ResizeExact(this->NumTris, 3 * this->NumTris);

    // Copy local edges to global edge array. Add in the originating edge id
    // used later when merging.
    EdgeVectorType emptyVector;
    this->Edges = new MergeTuple<IDType, float>[3 * this->NumTris]; // three edges per triangle
    svtkIdType edgeNum = 0;
    for (auto ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      auto eEnd = (*ldItr).LocalEdges.end();
      for (auto eItr = (*ldItr).LocalEdges.begin(); eItr != eEnd; ++eItr)
      {
        this->Edges[edgeNum].V0 = eItr->V0;
        this->Edges[edgeNum].V1 = eItr->V1;
        this->Edges[edgeNum].T = eItr->T;
        this->Edges[edgeNum].EId = edgeNum;
        edgeNum++;
      }
      (*ldItr).LocalEdges.swap(emptyVector); // frees memory
    }                                        // For all threads
  }                                          // Reduce
};                                           // ExtractEdgesBase

// Traverse all cells and extract intersected edges (without a sphere tree).
template <typename IDType, typename TIP>
struct ExtractEdges : public ExtractEdgesBase<IDType, TIP>
{
  const unsigned char* InOut;

  ExtractEdges(TIP* inPts, CellIter* c, svtkPlane* plane, unsigned char* inout, svtkCellArray* tris)
    : ExtractEdgesBase<IDType, TIP>(inPts, c, plane, tris)
    , InOut(inout)
  {
  }

  // Set up the iteration process
  void Initialize() { this->ExtractEdgesBase<IDType, TIP>::Initialize(); }

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
    const TIP* x;
    double s[MAX_CELL_VERTS], deltaScalar, xp[3];
    float t;
    unsigned char v0, v1;
    const unsigned char* inout = this->InOut;

    for (; cellId < endCellId; ++cellId)
    {
      // Does the plane cut this cell?
      if (Classify::PlaneIntersects(inout, cellIter->NumVerts, c))
      {
        // Compute case by repeated masking with function value
        for (isoCase = 0, i = 0; i < cellIter->NumVerts; ++i)
        {
          x = this->InPts + 3 * c[i];
          xp[0] = static_cast<double>(x[0]);
          xp[1] = static_cast<double>(x[1]);
          xp[2] = static_cast<double>(x[2]);
          s[i] = svtkPlane::Evaluate(this->Normal, this->Origin, xp);
          isoCase |= (s[i] >= 0.0 ? BaseCell::Mask[i] : 0);
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
            t = (deltaScalar == 0.0 ? 0.0 : (-s[v0] / deltaScalar));
            t = (c[v0] < c[v1] ? t : (1.0 - t));  // edges (v0,v1) must have v0<v1
            lEdges.emplace_back(c[v0], c[v1], t); // edge constructor may swap v0<->v1
          }                                       // for all edges in this case
        }                                         // if contour passes through this cell
      }                                           // if plane intersects
      c = cellIter->Next();                       // move to the next cell
    }                                             // for all cells in this batch
  }

  // Composite local thread data
  void Reduce() { this->ExtractEdgesBase<IDType, TIP>::Reduce(); } // Reduce
};                                                                 // ExtractEdges

// Produce points for non-merged points from input edge tuples. Every edge
// produces one point; three edges in a row form a triangle. The merge edges
// contain a interpolation parameter t used to interpolate point oordinates.
// into the final SVTK points array. The template parameters correspond to the
// type of input and output points.
template <typename TIP, typename TOP, typename IDType>
struct ProducePoints
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTuple<IDType, float>* Edges;
  const TIP* InPts;
  TOP* OutPts;

  ProducePoints(const MergeTuple<IDType, float>* mt, const TIP* inPts, TOP* outPts)
    : Edges(mt)
    , InPts(inPts)
    , OutPts(outPts)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const MergeTuple<IDType, float>* mergeTuple;
    const TIP *x0, *x1, *inPts = this->InPts;
    TOP *x, *outPts = this->OutPts;
    IDType v0, v1;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->Edges + ptId;
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

// Functor to build the SVTK triangle list in parallel from the generated,
// non-merged edges. Every three edges represents one triangle.
struct ProduceTriangles
{
  svtkCellArray* Tris;

  ProduceTriangles(svtkCellArray* tris)
    : Tris(tris)
  {
  }

  struct Impl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, svtkIdType triId, svtkIdType endTriId)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      auto offsetRange = svtk::DataArrayValueRange<1>(offsets, triId, endTriId + 1);
      ValueType offset = 3 * (triId - 1); // Incremented before first use
      std::generate(
        offsetRange.begin(), offsetRange.end(), [&]() -> ValueType { return offset += 3; });

      auto connRange = svtk::DataArrayValueRange<1>(conn, 3 * triId, 3 * endTriId);
      svtkIdType ptId = 3 * triId;
      std::iota(connRange.begin(), connRange.end(), ptId);
    }
  };

  void operator()(svtkIdType triId, svtkIdType endTriId)
  {
    this->Tris->Visit(Impl{}, triId, endTriId);
  }
};

// If requested, interpolate point data attributes from non-merged
// points. The merge tuple contains an interpolation value t for the merged
// edge. Templated on type of id.
template <typename TIds>
struct ProduceAttributes
{
  const MergeTuple<TIds, float>* Edges; // all edges
  ArrayList* Arrays;                    // the list of attributes to interpolate

  ProduceAttributes(const MergeTuple<TIds, float>* mt, ArrayList* arrays)
    : Edges(mt)
    , Arrays(arrays)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const MergeTuple<TIds, float>* mergeTuple;
    TIds v0, v1;
    float t;

    for (; ptId < endPtId; ++ptId)
    {
      mergeTuple = this->Edges + ptId;
      v0 = mergeTuple->V0;
      v1 = mergeTuple->V1;
      t = mergeTuple->T;
      this->Arrays->InterpolateEdge(v0, v1, t, ptId);
    }
  }
};

// This method generates the output isosurface triangle connectivity list.
template <typename IDType>
struct ProduceMergedTriangles
{
  typedef MergeTuple<IDType, float> MergeTupleType;

  const MergeTupleType* MergeArray;
  const IDType* Offsets;
  svtkIdType NumTris;
  svtkCellArray* Tris;
  int NumThreadsUsed; // placeholder

  ProduceMergedTriangles(
    const MergeTupleType* merge, const IDType* offsets, svtkIdType numTris, svtkCellArray* tris)
    : MergeArray(merge)
    , Offsets(offsets)
    , NumTris(numTris)
    , Tris(tris)
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
      const IDType* offsets, const MergeTupleType* mergeArray)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* conn = state.GetConnectivity();

      for (; ptId < endPtId; ++ptId)
      {
        const IDType numPtsInGroup = offsets[ptId + 1] - offsets[ptId];
        for (IDType i = 0; i < numPtsInGroup; ++i)
        {
          const IDType connIdx = mergeArray[offsets[ptId] + i].EId;
          conn->SetValue(connIdx, static_cast<ValueType>(ptId));
        } // for this group of coincident edges
      }   // for all merged points
    }
  };

  // Loop over all merged points and update the ids of the triangle
  // connectivity.  Offsets point to the beginning of a group of equal edges:
  // all edges in the group are updated to the current merged point id.
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    this->Tris->Visit(Impl{}, ptId, endPtId, this->Offsets, this->MergeArray);
  }

  struct ReduceImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, const svtkIdType numTris)
    {
      using ValueType = typename CellStateT::ValueType;

      auto offsets = svtk::DataArrayValueRange<1>(state.GetOffsets(), 0, numTris + 1);
      ValueType offset = -3; // +=3 on first access
      std::generate(offsets.begin(), offsets.end(), [&]() -> ValueType { return offset += 3; });
    }
  };

  // Update the triangle connectivity (numPts for each triangle. This could
  // be done in parallel but it's probably not faster.
  void Reduce() { this->Tris->Visit(ReduceImpl{}, this->NumTris); }
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

  ProduceMergedPoints(const MergeTupleType* merge, const IDType* offsets, TIP* inPts, TOP* outPts)
    : MergeArray(merge)
    , Offsets(offsets)
    , InPts(inPts)
    , OutPts(outPts)
  {
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
struct ProduceMergedAttributes
{
  const MergeTuple<TIds, float>* Edges; // all edges, sorted into groups of merged edges
  const TIds* Offsets;                  // refer to single, unique, merged edge
  ArrayList* Arrays;                    // carry list of attributes to interpolate

  ProduceMergedAttributes(const MergeTuple<TIds, float>* mt, const TIds* offsets, ArrayList* arrays)
    : Edges(mt)
    , Offsets(offsets)
    , Arrays(arrays)
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
      this->Arrays->InterpolateEdge(v0, v1, t, ptId);
    }
  }
};

// Wrapper to handle multiple template types for generating intersected edges
template <typename TIds>
int ProcessEdges(svtkIdType numCells, svtkPoints* inPts, CellIter* cellIter, svtkPlane* plane,
  unsigned char* inout, svtkPoints* outPts, svtkCellArray* newPolys, svtkTypeBool mergePts,
  svtkTypeBool intAttr, svtkPointData* inPD, svtkPointData* outPD, svtkTypeBool seqProcessing,
  int& numThreads)
{
  // Extract edges that the plane intersects.
  svtkIdType numTris = 0;
  MergeTuple<TIds, float>* mergeEdges = nullptr; // may need reference counting

  // Extract edges
  int ptsType = inPts->GetDataType();
  void* pts = inPts->GetVoidPointer(0);
  if (ptsType == SVTK_FLOAT)
  {
    ExtractEdges<TIds, float> extractEdges((float*)pts, cellIter, plane, inout, newPolys);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);
    numTris = extractEdges.NumTris;
    mergeEdges = extractEdges.Edges;
  }
  else // if (ptsType == SVTK_DOUBLE)
  {
    ExtractEdges<TIds, double> extractEdges((double*)pts, cellIter, plane, inout, newPolys);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numCells, extractEdges, numThreads);
    numTris = extractEdges.NumTris;
    mergeEdges = extractEdges.Edges;
  }
  int nt = numThreads;

  // Make sure data was produced
  if (numTris <= 0)
  {
    outPts->SetNumberOfPoints(0);
    delete[] mergeEdges;
    return 1;
  }

  // There are two ways forward: do not merge coincident points; or merge
  // points. Merging typically takes longer, while the output size of
  // unmerged points is larger.
  int inPtsType = inPts->GetDataType();
  void* inPtsPtr = inPts->GetVoidPointer(0);
  int outPtsType = outPts->GetDataType();
  void* outPtsPtr;

  if (!mergePts)
  {
    // Produce non-merged points from edges. Each edge produces one point;
    // three edges define an output triangle.
    svtkIdType numPts = 3 * numTris;
    outPts->GetData()->WriteVoidPointer(0, 3 * numPts);
    outPtsPtr = outPts->GetVoidPointer(0);

    if (inPtsType == SVTK_FLOAT)
    {
      if (outPtsType == SVTK_FLOAT)
      {
        ProducePoints<float, float, TIds> producePoints(
          mergeEdges, (float*)inPtsPtr, (float*)outPtsPtr);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
      else // outPtsType == SVTK_DOUBLE
      {
        ProducePoints<float, double, TIds> producePoints(
          mergeEdges, (float*)inPtsPtr, (double*)outPtsPtr);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
    }
    else // inPtsType == SVTK_DOUBLE
    {
      if (outPtsType == SVTK_FLOAT)
      {
        ProducePoints<double, float, TIds> producePoints(
          mergeEdges, (double*)inPtsPtr, (float*)outPtsPtr);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
      else // outPtsType == SVTK_DOUBLE
      {
        ProducePoints<double, double, TIds> producePoints(
          mergeEdges, (double*)inPtsPtr, (double*)outPtsPtr);
        EXECUTE_SMPFOR(seqProcessing, numPts, producePoints);
      }
    }

    // Produce non-merged triangles from edges
    ProduceTriangles produceTris(newPolys);
    EXECUTE_SMPFOR(seqProcessing, numTris, produceTris);

    // Interpolate attributes if requested
    if (intAttr)
    {
      ArrayList arrays;
      outPD->InterpolateAllocate(inPD, numPts);
      arrays.AddArrays(numPts, inPD, outPD);
      ProduceAttributes<TIds> interpolate(mergeEdges, &arrays);
      EXECUTE_SMPFOR(seqProcessing, numPts, interpolate);
    }
  }

  else // generate merged output
  {
    // Merge coincident edges. The Offsets refer to the single unique edge
    // from the sorted group of duplicate edges.
    svtkIdType numPts;
    svtkStaticEdgeLocatorTemplate<TIds, float> loc;
    const TIds* offsets = loc.MergeEdges(3 * numTris, mergeEdges, numPts);

    // Generate triangles from merged edges.
    ProduceMergedTriangles<TIds> produceTris(mergeEdges, offsets, numTris, newPolys);
    EXECUTE_REDUCED_SMPFOR(seqProcessing, numPts, produceTris, numThreads);
    numThreads = nt;

    // Generate points (one per unique edge)
    outPts->GetData()->WriteVoidPointer(0, 3 * numPts);
    outPtsPtr = outPts->GetVoidPointer(0);

    // Only handle combinations of real types
    if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_FLOAT)
    {
      ProduceMergedPoints<float, float, TIds> producePts(
        mergeEdges, offsets, (float*)inPtsPtr, (float*)outPtsPtr);
      EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
    }
    else if (inPtsType == SVTK_DOUBLE && outPtsType == SVTK_DOUBLE)
    {
      ProduceMergedPoints<double, double, TIds> producePts(
        mergeEdges, offsets, (double*)inPtsPtr, (double*)outPtsPtr);
      EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
    }
    else if (inPtsType == SVTK_FLOAT && outPtsType == SVTK_DOUBLE)
    {
      ProduceMergedPoints<float, double, TIds> producePts(
        mergeEdges, offsets, (float*)inPtsPtr, (double*)outPtsPtr);
      EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
    }
    else // if ( inPtsType == SVTK_DOUBLE && outPtsType == SVTK_FLOAT )
    {
      ProduceMergedPoints<double, float, TIds> producePts(
        mergeEdges, offsets, (double*)inPtsPtr, (float*)outPtsPtr);
      EXECUTE_SMPFOR(seqProcessing, numPts, producePts);
    }

    // Now process point data attributes if requested
    if (intAttr)
    {
      ArrayList arrays;
      outPD->InterpolateAllocate(inPD, numPts);
      arrays.AddArrays(numPts, inPD, outPD);
      ProduceMergedAttributes<TIds> interpolate(mergeEdges, offsets, &arrays);
      EXECUTE_SMPFOR(seqProcessing, numPts, interpolate);
    }
  }

  // Clean up
  delete[] mergeEdges;
  return 1;
}

// Functor for assigning normals at each point
struct ComputePointNormals
{
  float Normal[3];
  float* PointNormals;

  ComputePointNormals(float normal[3], float* ptNormals)
    : PointNormals(ptNormals)
  {
    this->Normal[0] = normal[0];
    this->Normal[1] = normal[1];
    this->Normal[2] = normal[2];
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    float* n = this->PointNormals + 3 * ptId;

    for (; ptId < endPtId; ++ptId, n += 3)
    {
      n[0] = this->Normal[0];
      n[1] = this->Normal[1];
      n[2] = this->Normal[2];
    }
  }

  static void Execute(svtkTypeBool seqProcessing, svtkPoints* pts, svtkPlane* plane, svtkPointData* pd)
  {
    svtkIdType numPts = pts->GetNumberOfPoints();

    svtkFloatArray* ptNormals = svtkFloatArray::New();
    ptNormals->SetName("Normals");
    ptNormals->SetNumberOfComponents(3);
    ptNormals->SetNumberOfTuples(numPts);
    float* ptN = static_cast<float*>(ptNormals->GetVoidPointer(0));

    // Get the normal
    double dn[3];
    plane->GetNormal(dn);
    svtkMath::Normalize(dn);
    float n[3];
    n[0] = static_cast<float>(dn[0]);
    n[1] = static_cast<float>(dn[1]);
    n[2] = static_cast<float>(dn[2]);

    // Process all points, averaging normals
    ComputePointNormals compute(n, ptN);
    EXECUTE_SMPFOR(seqProcessing, numPts, compute);

    // Clean up and get out
    pd->SetNormals(ptNormals);
    ptNormals->Delete();
  }
};

} // anonymous namespace

//-----------------------------------------------------------------------------
// Construct an instance of the class.
svtk3DLinearGridPlaneCutter::svtk3DLinearGridPlaneCutter()
{
  this->Plane = svtkPlane::New();
  this->MergePoints = false;
  this->InterpolateAttributes = true;
  this->ComputeNormals = false;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->SequentialProcessing = false;
  this->NumberOfThreadsUsed = 0;
  this->LargeIds = false;
}

//-----------------------------------------------------------------------------
svtk3DLinearGridPlaneCutter::~svtk3DLinearGridPlaneCutter()
{
  this->SetPlane(nullptr);
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
svtkMTimeType svtk3DLinearGridPlaneCutter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->Plane != nullptr)
  {
    svtkMTimeType mTime2 = this->Plane->GetMTime();
    return (mTime2 > mTime ? mTime2 : mTime);
  }
  else
  {
    return mTime;
  }
}

//-----------------------------------------------------------------------------
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int svtk3DLinearGridPlaneCutter::ProcessPiece(
  svtkUnstructuredGrid* input, svtkPlane* plane, svtkPolyData* output)
{
  // Make sure there is input data to process
  svtkPoints* inPts = input->GetPoints();
  svtkIdType numPts = inPts->GetNumberOfPoints();
  svtkCellArray* cells = input->GetCells();
  svtkIdType numCells = cells->GetNumberOfCells();
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

  // Output triangles go here.
  svtkCellArray* newPolys = svtkCellArray::New();

  // Set up the cells for processing. A specialized iterator is used to traverse the cells.
  unsigned char* cellTypes =
    static_cast<unsigned char*>(input->GetCellTypesArray()->GetVoidPointer(0));
  CellIter* cellIter = new CellIter(numCells, cellTypes, cells);

  // Compute plane-cut scalars
  unsigned char* inout = nullptr;
  int ptsType = inPts->GetDataType();
  if (ptsType == SVTK_FLOAT)
  {
    ClassifyPoints<float> classify(inPts, plane);
    svtkSMPTools::For(0, numPts, classify);
    inout = classify.InOutArray;
  }
  else if (ptsType == SVTK_DOUBLE)
  {
    ClassifyPoints<double> classify(inPts, plane);
    svtkSMPTools::For(0, numPts, classify);
    inout = classify.InOutArray;
  }

  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();

  // Determine the size/type of point and cell ids needed to index points
  // and cells. Using smaller ids results in a greatly reduced memory footprint
  // and faster processing.
  this->LargeIds = (numPts >= SVTK_INT_MAX || numCells >= SVTK_INT_MAX ? true : false);

  // Generate all of the merged points and triangles
  if (this->LargeIds == false)
  {
    if (!ProcessEdges<int>(numCells, inPts, cellIter, plane, inout, outPts, newPolys,
          this->MergePoints, this->InterpolateAttributes, inPD, outPD, this->SequentialProcessing,
          this->NumberOfThreadsUsed))
    {
      return 0;
    }
  }
  else
  {
    if (!ProcessEdges<svtkIdType>(numCells, inPts, cellIter, plane, inout, outPts, newPolys,
          this->MergePoints, this->InterpolateAttributes, inPD, outPD, this->SequentialProcessing,
          this->NumberOfThreadsUsed))
    {
      return 0;
    }
  }

  // If requested, compute point normals. Just set the point normals to the
  // plane normal.
  if (this->ComputeNormals)
  {
    ComputePointNormals::Execute(this->SequentialProcessing, outPts, plane, outPD);
  }

  // Report the results of execution
  svtkLog(TRACE,
    "Created: " << outPts->GetNumberOfPoints() << " points, " << newPolys->GetNumberOfCells()
                << " triangles");

  // Clean up
  if (inout != nullptr)
  {
    delete[] inout;
  }
  delete cellIter;
  output->SetPoints(outPts);
  outPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// The output dataset type varies dependingon the input type.
int svtk3DLinearGridPlaneCutter::RequestDataObject(
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

  svtkLog(ERROR, "Not sure what type of output to create!");
  return 0;
}

//-----------------------------------------------------------------------------
// Specialized plane cutting filter to handle unstructured grids with 3D
// linear cells (tetrahedras, hexes, wedges, pyradmids, voxels)
//
int svtk3DLinearGridPlaneCutter::RequestData(
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

  // Need a plane to do the cutting
  svtkPlane* plane = this->Plane;
  if (!plane)
  {
    svtkLog(ERROR, "Cut plane not defined");
    return 0;
  }

  // If the input is an unstructured grid, then simply process this single
  // grid producing a single output svtkPolyData.
  if (inputGrid)
  {
    this->ProcessPiece(inputGrid, plane, outputPolyData);
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
        polydata = svtkPolyData::New();
        this->ProcessPiece(grid, plane, polydata);
        outputMBDS->SetDataSet(inIter, polydata);
        polydata->Delete();
      }
      else
      {
        svtkLog(INFO, "This filter only processes unstructured grids");
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtk3DLinearGridPlaneCutter::SetOutputPointsPrecision(int precision)
{
  this->OutputPointsPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtk3DLinearGridPlaneCutter::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}

//-----------------------------------------------------------------------------
bool svtk3DLinearGridPlaneCutter::CanFullyProcessDataObject(svtkDataObject* object)
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
int svtk3DLinearGridPlaneCutter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void svtk3DLinearGridPlaneCutter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << "\n";

  os << indent << "Merge Points: " << (this->MergePoints ? "true\n" : "false\n");
  os << indent
     << "Interpolate Attributes: " << (this->InterpolateAttributes ? "true\n" : "false\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "true\n" : "false\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";

  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Large Ids: " << (this->LargeIds ? "true\n" : "false\n");
}

#undef EXECUTE_SMPFOR
#undef EXECUTE_REDUCED_SMPFOR
#undef MAX_CELL_VERTS
