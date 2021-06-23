//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_OnBoundary_h
#define svtk_m_exec_arg_OnBoundary_h

#include <svtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting if a point is a boundary point.
///
/// The \c AspectTagBoundary aspect tag causes the \c Fetch class to obtain
/// if the point is on a boundary.
///
struct AspectTagBoundary
{
};


/// \brief The \c ExecutionSignature tag to get if executing on a boundary element
///
struct Boundary : svtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr svtkm::IdComponent INDEX = 1;
  using AspectTag = svtkm::exec::arg::AspectTagBoundary;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag,
             svtkm::exec::arg::AspectTagBoundary,
             svtkm::exec::arg::ThreadIndicesPointNeighborhood,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesPointNeighborhood;

  using ValueType = svtkm::exec::BoundaryState;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetBoundaryState();
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_OnBoundary_h
