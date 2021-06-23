//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Keys_hxx
#define svtk_m_worklet_Keys_hxx

#include <svtkm/worklet/Keys.h>

namespace svtkm
{
namespace worklet
{
/// Build the internal arrays without modifying the input. This is more
/// efficient for stable sorted arrays, but requires an extra copy of the
/// keys for unstable sorting.
template <typename T>
template <typename KeyArrayType>
SVTKM_CONT void Keys<T>::BuildArrays(const KeyArrayType& keys,
                                    KeysSortType sort,
                                    svtkm::cont::DeviceAdapterId device)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf, "Keys::BuildArrays");

  switch (sort)
  {
    case KeysSortType::Unstable:
    {
      KeyArrayHandleType mutableKeys;
      svtkm::cont::Algorithm::Copy(device, keys, mutableKeys);

      this->BuildArraysInternal(mutableKeys, device);
    }
    break;
    case KeysSortType::Stable:
      this->BuildArraysInternalStable(keys, device);
      break;
  }
}

/// Build the internal arrays and also sort the input keys. This is more
/// efficient for unstable sorting, but requires an extra copy for stable
/// sorting.
template <typename T>
template <typename KeyArrayType>
SVTKM_CONT void Keys<T>::BuildArraysInPlace(KeyArrayType& keys,
                                           KeysSortType sort,
                                           svtkm::cont::DeviceAdapterId device)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf, "Keys::BuildArraysInPlace");

  switch (sort)
  {
    case KeysSortType::Unstable:
      this->BuildArraysInternal(keys, device);
      break;
    case KeysSortType::Stable:
    {
      this->BuildArraysInternalStable(keys, device);
      KeyArrayHandleType tmp;
      // Copy into a temporary array so that the permutation array copy
      // won't alias input/output memory:
      svtkm::cont::Algorithm::Copy(device, keys, tmp);
      svtkm::cont::Algorithm::Copy(
        device, svtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, tmp), keys);
    }
    break;
  }
}

template <typename T>
template <typename KeyArrayType>
SVTKM_CONT void Keys<T>::BuildArraysInternal(KeyArrayType& keys, svtkm::cont::DeviceAdapterId device)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternal");

  const svtkm::Id numKeys = keys.GetNumberOfValues();

  svtkm::cont::Algorithm::Copy(device, svtkm::cont::ArrayHandleIndex(numKeys), this->SortedValuesMap);

  // TODO: Do we need the ability to specify a comparison functor for sort?
  svtkm::cont::Algorithm::SortByKey(device, keys, this->SortedValuesMap);

  // Find the unique keys and the number of values per key.
  svtkm::cont::Algorithm::ReduceByKey(device,
                                     keys,
                                     svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(1, numKeys),
                                     this->UniqueKeys,
                                     this->Counts,
                                     svtkm::Sum());

  // Get the offsets from the counts with a scan.
  svtkm::Id offsetsTotal = svtkm::cont::Algorithm::ScanExclusive(
    device, svtkm::cont::make_ArrayHandleCast(this->Counts, svtkm::Id()), this->Offsets);
  SVTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
  (void)offsetsTotal;                   // Shut up, compiler
}

template <typename T>
template <typename KeyArrayType>
SVTKM_CONT void Keys<T>::BuildArraysInternalStable(const KeyArrayType& keys,
                                                  svtkm::cont::DeviceAdapterId device)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf, "Keys::BuildArraysInternalStable");

  const svtkm::Id numKeys = keys.GetNumberOfValues();

  // Produce a stable sorted map of the keys:
  this->SortedValuesMap = StableSortIndices::Sort(device, keys);
  auto sortedKeys = svtkm::cont::make_ArrayHandlePermutation(this->SortedValuesMap, keys);

  // Find the unique keys and the number of values per key.
  svtkm::cont::Algorithm::ReduceByKey(device,
                                     sortedKeys,
                                     svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(1, numKeys),
                                     this->UniqueKeys,
                                     this->Counts,
                                     svtkm::Sum());

  // Get the offsets from the counts with a scan.
  svtkm::Id offsetsTotal = svtkm::cont::Algorithm::ScanExclusive(
    svtkm::cont::make_ArrayHandleCast(this->Counts, svtkm::Id()), this->Offsets);
  SVTKM_ASSERT(offsetsTotal == numKeys); // Sanity check
  (void)offsetsTotal;                   // Shut up, compiler
}
}
}
#endif
