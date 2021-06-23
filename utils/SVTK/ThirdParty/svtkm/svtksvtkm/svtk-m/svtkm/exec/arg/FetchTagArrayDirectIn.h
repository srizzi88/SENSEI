//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagArrayDirectIn_h
#define svtk_m_exec_arg_FetchTagArrayDirectIn_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values with direct indexing.
///
/// \c FetchTagArrayDirectIn is a tag used with the \c Fetch class to retrieve
/// values from an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Load is used as the index into the array.
///
struct FetchTagArrayDirectIn
{
};


SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline SVTKM_EXEC T load(const U& u, svtkm::Id v)
{
  return u.Get(v);
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline SVTKM_EXEC T load(const U* u, svtkm::Id v)
{
  return u->Get(v);
}

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<svtkm::exec::arg::FetchTagArrayDirectIn,
             svtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  //need to remove pointer type from ThreadIdicesType
  using ET = typename std::remove_const<typename std::remove_pointer<ExecObjectType>::type>::type;
  using PortalType =
    typename std::conditional<std::is_pointer<ExecObjectType>::value, const ET*, const ET&>::type;

  using ValueType = typename ET::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, PortalType arrayPortal) const
  {
    return load<ValueType>(arrayPortal, indices.GetInputIndex());
  }

  SVTKM_EXEC
  void Store(const ThreadIndicesType&, PortalType, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace svtkm::exec::arg

#endif //svtk_m_exec_arg_FetchTagArrayDirectIn_h
