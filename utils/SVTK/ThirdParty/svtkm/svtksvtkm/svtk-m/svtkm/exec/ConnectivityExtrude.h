//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_ConnectivityExtrude_h
#define svtk_m_exec_ConnectivityExtrude_h

#include <svtkm/internal/IndicesExtrude.h>

#include <svtkm/CellShape.h>
#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h>


namespace svtkm
{
namespace exec
{

template <typename Device>
class SVTKM_ALWAYS_EXPORT ConnectivityExtrude
{
private:
  using Int32HandleType = svtkm::cont::ArrayHandle<svtkm::Int32>;
  using Int32PortalType = typename Int32HandleType::template ExecutionTypes<Device>::PortalConst;

public:
  using ConnectivityPortalType = Int32PortalType;
  using NextNodePortalType = Int32PortalType;

  using SchedulingRangeType = svtkm::Id2;

  using CellShapeTag = svtkm::CellShapeTagWedge;

  using IndicesType = IndicesExtrude;

  ConnectivityExtrude() = default;

  ConnectivityExtrude(const ConnectivityPortalType& conn,
                      const NextNodePortalType& nextnode,
                      svtkm::Int32 cellsPerPlane,
                      svtkm::Int32 pointsPerPlane,
                      svtkm::Int32 numPlanes,
                      bool periodic);

  SVTKM_EXEC
  svtkm::Id GetNumberOfElements() const { return this->NumberOfCells; }

  SVTKM_EXEC
  CellShapeTag GetCellShape(svtkm::Id) const { return svtkm::CellShapeTagWedge(); }

  SVTKM_EXEC
  IndicesType GetIndices(svtkm::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  SVTKM_EXEC
  IndicesType GetIndices(const svtkm::Id2& index) const;
  template <typename IndexType>
  SVTKM_EXEC svtkm::IdComponent GetNumberOfIndices(const IndexType& svtkmNotUsed(index)) const
  {
    return 6;
  }

  SVTKM_EXEC
  svtkm::Id LogicalToFlatToIndex(const svtkm::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfCellsPerPlane);
  };

  SVTKM_EXEC
  svtkm::Id2 FlatToLogicalToIndex(svtkm::Id index) const
  {
    const svtkm::Id cellId = index % this->NumberOfCellsPerPlane;
    const svtkm::Id plane = index / this->NumberOfCellsPerPlane;
    return svtkm::Id2(cellId, plane);
  }

private:
  ConnectivityPortalType Connectivity;
  NextNodePortalType NextNode;
  svtkm::Int32 NumberOfCellsPerPlane;
  svtkm::Int32 NumberOfPointsPerPlane;
  svtkm::Int32 NumberOfPlanes;
  svtkm::Id NumberOfCells;
};


template <typename Device>
class ReverseConnectivityExtrude
{
private:
  using Int32HandleType = svtkm::cont::ArrayHandle<svtkm::Int32>;
  using Int32PortalType = typename Int32HandleType::template ExecutionTypes<Device>::PortalConst;

public:
  using ConnectivityPortalType = Int32PortalType;
  using OffsetsPortalType = Int32PortalType;
  using CountsPortalType = Int32PortalType;
  using PrevNodePortalType = Int32PortalType;

  using SchedulingRangeType = svtkm::Id2;

  using CellShapeTag = svtkm::CellShapeTagVertex;

  using IndicesType = ReverseIndicesExtrude<ConnectivityPortalType>;

  ReverseConnectivityExtrude() = default;

  SVTKM_EXEC
  ReverseConnectivityExtrude(const ConnectivityPortalType& conn,
                             const OffsetsPortalType& offsets,
                             const CountsPortalType& counts,
                             const PrevNodePortalType& prevNode,
                             svtkm::Int32 cellsPerPlane,
                             svtkm::Int32 pointsPerPlane,
                             svtkm::Int32 numPlanes);

  SVTKM_EXEC
  svtkm::Id GetNumberOfElements() const
  {
    return this->NumberOfPointsPerPlane * this->NumberOfPlanes;
  }

  SVTKM_EXEC
  CellShapeTag GetCellShape(svtkm::Id) const { return svtkm::CellShapeTagVertex(); }

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  SVTKM_EXEC
  IndicesType GetIndices(svtkm::Id index) const
  {
    return this->GetIndices(this->FlatToLogicalToIndex(index));
  }

  SVTKM_EXEC
  IndicesType GetIndices(const svtkm::Id2& index) const;

  template <typename IndexType>
  SVTKM_EXEC svtkm::IdComponent GetNumberOfIndices(const IndexType& svtkmNotUsed(index)) const
  {
    return 1;
  }

  SVTKM_EXEC
  svtkm::Id LogicalToFlatToIndex(const svtkm::Id2& index) const
  {
    return index[0] + (index[1] * this->NumberOfPointsPerPlane);
  };

  SVTKM_EXEC
  svtkm::Id2 FlatToLogicalToIndex(svtkm::Id index) const
  {
    const svtkm::Id vertId = index % this->NumberOfPointsPerPlane;
    const svtkm::Id plane = index / this->NumberOfPointsPerPlane;
    return svtkm::Id2(vertId, plane);
  }

  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
  CountsPortalType Counts;
  PrevNodePortalType PrevNode;
  svtkm::Int32 NumberOfCellsPerPlane;
  svtkm::Int32 NumberOfPointsPerPlane;
  svtkm::Int32 NumberOfPlanes;
};


template <typename Device>
ConnectivityExtrude<Device>::ConnectivityExtrude(const ConnectivityPortalType& conn,
                                                 const ConnectivityPortalType& nextNode,
                                                 svtkm::Int32 cellsPerPlane,
                                                 svtkm::Int32 pointsPerPlane,
                                                 svtkm::Int32 numPlanes,
                                                 bool periodic)
  : Connectivity(conn)
  , NextNode(nextNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
  this->NumberOfCells = periodic ? (static_cast<svtkm::Id>(cellsPerPlane) * numPlanes)
                                 : (static_cast<svtkm::Id>(cellsPerPlane) * (numPlanes - 1));
}

template <typename Device>
typename ConnectivityExtrude<Device>::IndicesType ConnectivityExtrude<Device>::GetIndices(
  const svtkm::Id2& index) const
{
  svtkm::Id tr = index[0];
  svtkm::Id p0 = index[1];
  svtkm::Id p1 = (p0 < (this->NumberOfPlanes - 1)) ? (p0 + 1) : 0;

  svtkm::Vec3i_32 pointIds1, pointIds2;
  for (int i = 0; i < 3; ++i)
  {
    pointIds1[i] = this->Connectivity.Get((tr * 3) + i);
    pointIds2[i] = this->NextNode.Get(pointIds1[i]);
  }

  return IndicesType(pointIds1,
                     static_cast<svtkm::Int32>(p0),
                     pointIds2,
                     static_cast<svtkm::Int32>(p1),
                     this->NumberOfPointsPerPlane);
}


template <typename Device>
ReverseConnectivityExtrude<Device>::ReverseConnectivityExtrude(const ConnectivityPortalType& conn,
                                                               const OffsetsPortalType& offsets,
                                                               const CountsPortalType& counts,
                                                               const PrevNodePortalType& prevNode,
                                                               svtkm::Int32 cellsPerPlane,
                                                               svtkm::Int32 pointsPerPlane,
                                                               svtkm::Int32 numPlanes)
  : Connectivity(conn)
  , Offsets(offsets)
  , Counts(counts)
  , PrevNode(prevNode)
  , NumberOfCellsPerPlane(cellsPerPlane)
  , NumberOfPointsPerPlane(pointsPerPlane)
  , NumberOfPlanes(numPlanes)
{
}

template <typename Device>
typename ReverseConnectivityExtrude<Device>::IndicesType
ReverseConnectivityExtrude<Device>::GetIndices(const svtkm::Id2& index) const
{
  auto ptCur = index[0];
  auto ptPre = this->PrevNode.Get(ptCur);
  auto plCur = index[1];
  auto plPre = (plCur == 0) ? (this->NumberOfPlanes - 1) : (plCur - 1);

  return IndicesType(this->Connectivity,
                     this->Offsets.Get(ptPre),
                     this->Counts.Get(ptPre),
                     this->Offsets.Get(ptCur),
                     this->Counts.Get(ptCur),
                     static_cast<svtkm::IdComponent>(plPre),
                     static_cast<svtkm::IdComponent>(plCur),
                     this->NumberOfCellsPerPlane);
}
}
} // namespace svtkm::exec
#endif
