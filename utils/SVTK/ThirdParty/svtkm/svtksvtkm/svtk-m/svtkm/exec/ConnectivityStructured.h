//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_exec_ConnectivityStructured_h
#define svtk_m_exec_ConnectivityStructured_h

#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>
#include <svtkm/internal/ConnectivityStructuredInternals.h>

namespace svtkm
{
namespace exec
{

template <typename VisitTopology, typename IncidentTopology, svtkm::IdComponent Dimension>
class ConnectivityStructured
{
  SVTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
  SVTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);

  using InternalsType = svtkm::internal::ConnectivityStructuredInternals<Dimension>;

  using Helper =
    svtkm::internal::ConnectivityStructuredIndexHelper<VisitTopology, IncidentTopology, Dimension>;

public:
  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  SVTKM_EXEC_CONT
  ConnectivityStructured()
    : Internals()
  {
  }

  SVTKM_EXEC_CONT
  ConnectivityStructured(const InternalsType& src)
    : Internals(src)
  {
  }

  SVTKM_EXEC_CONT
  ConnectivityStructured(const ConnectivityStructured& src)
    : Internals(src.Internals)
  {
  }

  SVTKM_EXEC_CONT
  ConnectivityStructured(
    const ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>& src)
    : Internals(src.Internals)
  {
  }


  ConnectivityStructured& operator=(const ConnectivityStructured& src) = default;
  ConnectivityStructured& operator=(ConnectivityStructured&& src) = default;


  SVTKM_EXEC
  svtkm::Id GetNumberOfElements() const { return Helper::GetNumberOfElements(this->Internals); }

  using CellShapeTag = typename Helper::CellShapeTag;
  SVTKM_EXEC
  CellShapeTag GetCellShape(svtkm::Id) const { return CellShapeTag(); }

  template <typename IndexType>
  SVTKM_EXEC svtkm::IdComponent GetNumberOfIndices(const IndexType& index) const
  {
    return Helper::GetNumberOfIndices(this->Internals, index);
  }

  using IndicesType = typename Helper::IndicesType;

  template <typename IndexType>
  SVTKM_EXEC IndicesType GetIndices(const IndexType& index) const
  {
    return Helper::GetIndices(this->Internals, index);
  }

  SVTKM_EXEC_CONT
  SchedulingRangeType FlatToLogicalFromIndex(svtkm::Id flatFromIndex) const
  {
    return Helper::FlatToLogicalFromIndex(this->Internals, flatFromIndex);
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatFromIndex(const SchedulingRangeType& logicalFromIndex) const
  {
    return Helper::LogicalToFlatFromIndex(this->Internals, logicalFromIndex);
  }

  SVTKM_EXEC_CONT
  SchedulingRangeType FlatToLogicalToIndex(svtkm::Id flatToIndex) const
  {
    return Helper::FlatToLogicalToIndex(this->Internals, flatToIndex);
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatToIndex(const SchedulingRangeType& logicalToIndex) const
  {
    return Helper::LogicalToFlatToIndex(this->Internals, logicalToIndex);
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, Dimension> GetPointDimensions() const
  {
    return this->Internals.GetPointDimensions();
  }

  SVTKM_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Internals.GetGlobalPointIndexStart();
  }

  friend class ConnectivityStructured<IncidentTopology, VisitTopology, Dimension>;

private:
  InternalsType Internals;
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_ConnectivityStructured_h
