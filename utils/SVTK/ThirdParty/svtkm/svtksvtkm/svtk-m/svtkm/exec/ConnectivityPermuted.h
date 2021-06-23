//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_exec_ConnectivityPermuted_h
#define svtk_m_exec_ConnectivityPermuted_h

#include <svtkm/CellShape.h>
#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>
#include <svtkm/VecFromPortal.h>

namespace svtkm
{
namespace exec
{

template <typename PermutationPortal, typename OriginalConnectivity>
class ConnectivityPermutedVisitCellsWithPoints
{
public:
  using SchedulingRangeType = typename OriginalConnectivity::SchedulingRangeType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints()
    : Portal()
    , Connectivity()
  {
  }

  SVTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints(const PermutationPortal& portal,
                                           const OriginalConnectivity& src)
    : Portal(portal)
    , Connectivity(src)
  {
  }

  SVTKM_EXEC_CONT
  ConnectivityPermutedVisitCellsWithPoints(const ConnectivityPermutedVisitCellsWithPoints& src)
    : Portal(src.Portal)
    , Connectivity(src.Connectivity)
  {
  }

  ConnectivityPermutedVisitCellsWithPoints& operator=(
    const ConnectivityPermutedVisitCellsWithPoints& src) = default;
  ConnectivityPermutedVisitCellsWithPoints& operator=(
    ConnectivityPermutedVisitCellsWithPoints&& src) = default;

  SVTKM_EXEC
  svtkm::Id GetNumberOfElements() const { return this->Portal.GetNumberOfValues(); }

  using CellShapeTag = typename OriginalConnectivity::CellShapeTag;

  SVTKM_EXEC
  CellShapeTag GetCellShape(svtkm::Id index) const
  {
    svtkm::Id pIndex = this->Portal.Get(index);
    return this->Connectivity.GetCellShape(pIndex);
  }

  SVTKM_EXEC
  svtkm::IdComponent GetNumberOfIndices(svtkm::Id index) const
  {
    return this->Connectivity.GetNumberOfIndices(this->Portal.Get(index));
  }

  using IndicesType = typename OriginalConnectivity::IndicesType;

  template <typename IndexType>
  SVTKM_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return this->Connectivity.GetIndices(this->Portal.Get(index));
  }

  PermutationPortal Portal;
  OriginalConnectivity Connectivity;
};

template <typename ConnectivityPortalType, typename OffsetPortalType>
class ConnectivityPermutedVisitPointsWithCells
{
public:
  using SchedulingRangeType = svtkm::Id;
  using IndicesType = svtkm::VecFromPortal<ConnectivityPortalType>;
  using CellShapeTag = svtkm::CellShapeTagVertex;

  ConnectivityPermutedVisitPointsWithCells() = default;

  ConnectivityPermutedVisitPointsWithCells(const ConnectivityPortalType& connectivity,
                                           const OffsetPortalType& offsets)
    : Connectivity(connectivity)
    , Offsets(offsets)
  {
  }

  SVTKM_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Offsets.GetNumberOfValues() - 1; }

  SVTKM_EXEC CellShapeTag GetCellShape(svtkm::Id) const { return CellShapeTag(); }

  SVTKM_EXEC
  svtkm::IdComponent GetNumberOfIndices(svtkm::Id index) const
  {
    const svtkm::Id offBegin = this->Offsets.Get(index);
    const svtkm::Id offEnd = this->Offsets.Get(index + 1);
    return static_cast<svtkm::IdComponent>(offEnd - offBegin);
  }

  SVTKM_EXEC IndicesType GetIndices(svtkm::Id index) const
  {
    const svtkm::Id offBegin = this->Offsets.Get(index);
    const svtkm::Id offEnd = this->Offsets.Get(index + 1);
    return IndicesType(
      this->Connectivity, static_cast<svtkm::IdComponent>(offEnd - offBegin), offBegin);
  }

private:
  ConnectivityPortalType Connectivity;
  OffsetPortalType Offsets;
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_ConnectivityPermuted_h
