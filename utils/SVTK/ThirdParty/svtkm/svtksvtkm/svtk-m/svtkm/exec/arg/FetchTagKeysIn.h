//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_arg_FetchTagKeysIn_h
#define svtk_m_exec_arg_FetchTagKeysIn_h

#include <svtkm/exec/arg/AspectTagDefault.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesReduceByKey.h>

#include <svtkm/exec/internal/ReduceByKeyLookup.h>

namespace svtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting key values in a reduce by key.
///
/// \c FetchTagKeysIn is a tag used with the \c Fetch class to retrieve keys
/// from the input domain of a reduce by keys worklet.
///
struct FetchTagKeysIn
{
};

template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct Fetch<
  svtkm::exec::arg::FetchTagKeysIn,
  svtkm::exec::arg::AspectTagDefault,
  svtkm::exec::arg::ThreadIndicesReduceByKey,
  svtkm::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>>
{
  using ThreadIndicesType = svtkm::exec::arg::ThreadIndicesReduceByKey;
  using ExecObjectType =
    svtkm::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>;

  using ValueType = typename ExecObjectType::KeyType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& keys) const
  {
    return keys.UniqueKeys.Get(indices.GetInputIndex());
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

#endif //svtk_m_exec_arg_FetchTagKeysIn_h
