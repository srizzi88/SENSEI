//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagArrayDirectOut_h
#define svtk_m_exec_arg_FetchTagArrayDirectOut_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for setting array values with direct indexing.
///
/// \c FetchTagArrayDirectOut is a tag used with the \c Fetch class to store
/// values in an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Store is used as the index into the array.
///
struct FetchTagArrayDirectOut
{
};

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagArrayDirectOut,
             svtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType&, const ExecObjectType&) const
  {
    // Load is a no-op for this fetch.
    return ValueType();
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  void Store(const ThreadIndicesType& indices,
             const ExecObjectType& arrayPortal,
             const ValueType& value) const
  {
    arrayPortal.Set(indices.GetOutputIndex(), value);
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_FetchTagArrayDirectOut_h
