//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_connectivity_CellSetDualGraph_h
#define svtk_m_worklet_connectivity_CellSetDualGraph_h

#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/exec/CellEdge.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
struct EdgeCount : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell numEdgesInCell);

  using ExecutionSignature = _2(CellShape, PointCount);

  using InputDomain = _1;

  template <typename CellShapeTag>
  SVTKM_EXEC svtkm::IdComponent operator()(CellShapeTag cellShape, svtkm::IdComponent pointCount) const
  {
    return svtkm::exec::CellEdgeNumberOfEdges(pointCount, cellShape, *this);
  }
};

struct EdgeExtract : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell cellIndices, FieldOutCell edgeIndices);

  using ExecutionSignature = void(CellShape, InputIndex, PointIndices, VisitIndex, _2, _3);

  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterCounting;

  template <typename CellShapeTag,
            typename CellIndexType,
            typename PointIndexVecType,
            typename EdgeIndexVecType>
  SVTKM_EXEC void operator()(CellShapeTag cellShape,
                            CellIndexType cellIndex,
                            const PointIndexVecType& pointIndices,
                            svtkm::IdComponent visitIndex,
                            CellIndexType& cellIndexOut,
                            EdgeIndexVecType& edgeIndices) const
  {
    cellIndexOut = cellIndex;
    edgeIndices = svtkm::exec::CellEdgeCanonicalId(
      pointIndices.GetNumberOfComponents(), visitIndex, cellShape, pointIndices, *this);
  }
};

struct CellToCellConnectivity : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn index,
                                WholeArrayIn cells,
                                WholeArrayOut from,
                                WholeArrayOut to);

  using ExecutionSignature = void(_1, InputIndex, _2, _3, _4);

  using InputDomain = _1;

  template <typename ConnectivityPortalType, typename CellIdPortalType>
  SVTKM_EXEC void operator()(svtkm::Id offset,
                            svtkm::Id index,
                            const CellIdPortalType& cells,
                            ConnectivityPortalType& from,
                            ConnectivityPortalType& to) const
  {
    from.Set(index * 2, cells.Get(offset));
    to.Set(index * 2, cells.Get(offset + 1));
    from.Set(index * 2 + 1, cells.Get(offset + 1));
    to.Set(index * 2 + 1, cells.Get(offset));
  }
};
} // svtkm::worklet::connectivity::detail

class CellSetDualGraph
{
public:
  using Algorithm = svtkm::cont::Algorithm;

  struct degree2
  {
    SVTKM_EXEC
    bool operator()(svtkm::Id degree) const { return degree >= 2; }
  };

  template <typename CellSet>
  void EdgeToCellConnectivity(const CellSet& cellSet,
                              svtkm::cont::ArrayHandle<svtkm::Id>& cellIds,
                              svtkm::cont::ArrayHandle<svtkm::Id2>& cellEdges) const
  {
    // Get number of edges for each cell and use it as scatter count.
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numEdgesPerCell;
    svtkm::worklet::DispatcherMapTopology<detail::EdgeCount> edgesPerCellDisp;
    edgesPerCellDisp.Invoke(cellSet, numEdgesPerCell);

    // Get uncompress Cell to Edge mapping
    svtkm::worklet::ScatterCounting scatter{ numEdgesPerCell };
    svtkm::worklet::DispatcherMapTopology<detail::EdgeExtract> edgeExtractDisp{ scatter };
    edgeExtractDisp.Invoke(cellSet, cellIds, cellEdges);
  }

  template <typename CellSetType>
  void Run(const CellSetType& cellSet,
           svtkm::cont::ArrayHandle<svtkm::Id>& numIndicesArray,
           svtkm::cont::ArrayHandle<svtkm::Id>& indexOffsetArray,
           svtkm::cont::ArrayHandle<svtkm::Id>& connectivityArray) const
  {
    // calculate the uncompressed Edge to Cell connectivity from Point to Cell connectivity
    // in the CellSet
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
    svtkm::cont::ArrayHandle<svtkm::Id2> cellEdges;
    EdgeToCellConnectivity(cellSet, cellIds, cellEdges);

    // sort cell ids by cell edges, this groups cells by cell edges
    Algorithm::SortByKey(cellEdges, cellIds);

    // count how many times an edge is shared by cells.
    svtkm::cont::ArrayHandle<svtkm::Id2> uniqueEdges;
    svtkm::cont::ArrayHandle<svtkm::Id> uniqueEdgeDegree;
    Algorithm::ReduceByKey(
      cellEdges,
      svtkm::cont::ArrayHandleConstant<svtkm::Id>(1, cellEdges.GetNumberOfValues()),
      uniqueEdges,
      uniqueEdgeDegree,
      svtkm::Add());

    // Extract edges shared by two cells
    svtkm::cont::ArrayHandle<svtkm::Id2> sharedEdges;
    Algorithm::CopyIf(uniqueEdges, uniqueEdgeDegree, sharedEdges, degree2());

    // find shared edges within all the edges.
    svtkm::cont::ArrayHandle<svtkm::Id> lb;
    Algorithm::LowerBounds(cellEdges, sharedEdges, lb);

    // take each shared edge and the cells to create 2 edges of the dual graph
    svtkm::cont::ArrayHandle<svtkm::Id> connFrom;
    svtkm::cont::ArrayHandle<svtkm::Id> connTo;
    connFrom.Allocate(sharedEdges.GetNumberOfValues() * 2);
    connTo.Allocate(sharedEdges.GetNumberOfValues() * 2);
    svtkm::worklet::DispatcherMapField<detail::CellToCellConnectivity> c2cDisp;
    c2cDisp.Invoke(lb, cellIds, connFrom, connTo);

    // Turn dual graph into Compressed Sparse Row format
    Algorithm::SortByKey(connFrom, connTo);
    Algorithm::Copy(connTo, connectivityArray);

    svtkm::cont::ArrayHandle<svtkm::Id> dualGraphVertices;
    Algorithm::ReduceByKey(
      connFrom,
      svtkm::cont::ArrayHandleConstant<svtkm::Id>(1, connFrom.GetNumberOfValues()),
      dualGraphVertices,
      numIndicesArray,
      svtkm::Add());
    Algorithm::ScanExclusive(numIndicesArray, indexOffsetArray);
  }
};
}
}
}

#endif //svtk_m_worklet_connectivity_CellSetDualGraph_h
