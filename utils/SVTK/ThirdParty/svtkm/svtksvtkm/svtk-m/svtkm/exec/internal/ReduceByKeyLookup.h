//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_internal_ReduceByKeyLookup_h
#define svtk_m_exec_internal_ReduceByKeyLookup_h

#include <svtkm/cont/ExecutionObjectBase.h>

#include <svtkm/StaticAssert.h>
#include <svtkm/Types.h>

#include <type_traits>

namespace svtkm
{
namespace exec
{
namespace internal
{

/// \brief Execution object holding lookup info for reduce by key.
///
/// A WorkletReduceByKey needs several arrays to map the current output object
/// to the respective key and group of values. This execution object holds that
/// state.
///
template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct ReduceByKeyLookup : svtkm::cont::ExecutionObjectBase
{
  using KeyType = typename KeyPortalType::ValueType;

  SVTKM_STATIC_ASSERT((std::is_same<typename IdPortalType::ValueType, svtkm::Id>::value));
  SVTKM_STATIC_ASSERT(
    (std::is_same<typename IdComponentPortalType::ValueType, svtkm::IdComponent>::value));

  KeyPortalType UniqueKeys;
  IdPortalType SortedValuesMap;
  IdPortalType Offsets;
  IdComponentPortalType Counts;

  SVTKM_EXEC_CONT
  ReduceByKeyLookup(const KeyPortalType& uniqueKeys,
                    const IdPortalType& sortedValuesMap,
                    const IdPortalType& offsets,
                    const IdComponentPortalType& counts)
    : UniqueKeys(uniqueKeys)
    , SortedValuesMap(sortedValuesMap)
    , Offsets(offsets)
    , Counts(counts)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ReduceByKeyLookup() {}
};
}
}
} // namespace svtkm::exec::internal

#endif //svtk_m_exec_internal_ReduceByKeyLookup_h
