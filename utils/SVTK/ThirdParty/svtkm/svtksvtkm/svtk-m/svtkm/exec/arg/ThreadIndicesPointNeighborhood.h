//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
#define svtk_m_exec_arg_ThreadIndicesPointNeighborhood_h

#include <svtkm/exec/BoundaryState.h>
#include <svtkm/exec/ConnectivityStructured.h>
#include <svtkm/exec/arg/ThreadIndicesBasic.h>
#include <svtkm/exec/arg/ThreadIndicesTopologyMap.h> //for Deflate and Inflate

#include <svtkm/Math.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

namespace detail
{
/// Given a \c Vec of (semi) arbitrary size, inflate it to a svtkm::Id3 by padding with zeros.
///
inline SVTKM_EXEC svtkm::Id3 To3D(svtkm::Id3 index)
{
  return index;
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a svtkm::Id3 by padding with zeros.
/// \overload
inline SVTKM_EXEC svtkm::Id3 To3D(svtkm::Id2 index)
{
  return svtkm::Id3(index[0], index[1], 1);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a svtkm::Id3 by padding with zeros.
/// \overload
inline SVTKM_EXEC svtkm::Id3 To3D(svtkm::Vec<svtkm::Id, 1> index)
{
  return svtkm::Id3(index[0], 1, 1);
}
}

/// \brief Container for thread information in a WorkletPointNeighborhood.
///
///
class ThreadIndicesPointNeighborhood
{

public:
  template <svtkm::IdComponent Dimension>
  SVTKM_EXEC ThreadIndicesPointNeighborhood(
    const svtkm::Id3& outIndex,
    const svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagPoint,
                                             svtkm::TopologyElementTagCell,
                                             Dimension>& connectivity,
    svtkm::Id globalThreadIndexOffset = 0)
    : State(outIndex, detail::To3D(connectivity.GetPointDimensions()))
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
    using ConnectivityType = svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagPoint,
                                                                svtkm::TopologyElementTagCell,
                                                                Dimension>;
    using ConnRangeType = typename ConnectivityType::SchedulingRangeType;
    const ConnRangeType index = detail::Deflate(outIndex, ConnRangeType());
    this->ThreadIndex = connectivity.LogicalToFlatToIndex(index);
    this->InputIndex = this->ThreadIndex;
    this->VisitIndex = 0;
    this->OutputIndex = this->ThreadIndex;
  }

  template <svtkm::IdComponent Dimension>
  SVTKM_EXEC ThreadIndicesPointNeighborhood(
    svtkm::Id threadIndex,
    svtkm::Id inputIndex,
    svtkm::IdComponent visitIndex,
    svtkm::Id outputIndex,
    const svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagPoint,
                                             svtkm::TopologyElementTagCell,
                                             Dimension>& connectivity,
    svtkm::Id globalThreadIndexOffset = 0)
    : State(detail::To3D(connectivity.FlatToLogicalToIndex(inputIndex)),
            detail::To3D(connectivity.GetPointDimensions()))
    , ThreadIndex(threadIndex)
    , InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
  }

  SVTKM_EXEC
  const svtkm::exec::BoundaryState& GetBoundaryState() const { return this->State; }

  SVTKM_EXEC
  svtkm::Id GetThreadIndex() const { return this->ThreadIndex; }

  SVTKM_EXEC
  svtkm::Id GetInputIndex() const { return this->InputIndex; }

  SVTKM_EXEC
  svtkm::Id3 GetInputIndex3D() const { return this->State.IJK; }

  SVTKM_EXEC
  svtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  SVTKM_EXEC
  svtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  /// \brief The global index (for streaming).
  ///
  /// Global index (for streaming)
  SVTKM_EXEC
  svtkm::Id GetGlobalIndex() const { return (this->GlobalThreadIndexOffset + this->OutputIndex); }

private:
  svtkm::exec::BoundaryState State;
  svtkm::Id ThreadIndex;
  svtkm::Id InputIndex;
  svtkm::Id OutputIndex;
  svtkm::IdComponent VisitIndex;
  svtkm::Id GlobalThreadIndexOffset;
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
