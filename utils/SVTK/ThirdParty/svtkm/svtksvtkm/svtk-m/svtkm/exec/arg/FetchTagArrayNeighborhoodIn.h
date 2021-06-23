//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
#define svtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h

#include <svtkm/exec/FieldNeighborhood.h>
#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting values of neighborhood around a point.
///
/// \c FetchTagArrayNeighborhoodIn is a tag used with the \c Fetch class to retrieve
/// values from an neighborhood.
///
struct FetchTagArrayNeighborhoodIn
{
};

template <typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagArrayNeighborhoodIn,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesPointNeighborhood,
             ExecObjectType>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesPointNeighborhood;
  using ValueType = svtkm::exec::FieldNeighborhood<ExecObjectType>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& arrayPortal) const
  {
    return ValueType(arrayPortal, indices.GetBoundaryState());
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
