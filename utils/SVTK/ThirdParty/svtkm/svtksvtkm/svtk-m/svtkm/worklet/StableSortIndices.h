//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_SortAndUniqueIndices_h
#define svtk_m_worklet_SortAndUniqueIndices_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ExecutionObjectBase.h>

namespace svtkm
{
namespace worklet
{

/// Produces an ArrayHandle<svtkm::Id> index array that stable-sorts and
/// optionally uniquifies an input array.
struct StableSortIndices
{
  using IndexArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;

  // Allows Sort to be called on an array that indexes into KeyPortal.
  // If the values compare equal, the indices are compared to stabilize the
  // result.
  template <typename KeyPortalType>
  struct IndirectSortPredicate
  {
    using KeyType = typename KeyPortalType::ValueType;

    const KeyPortalType KeyPortal;

    SVTKM_CONT
    IndirectSortPredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    SVTKM_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      // If the values compare equal, compare the indices as well so we get
      // consistent outputs.
      const KeyType valueA = this->KeyPortal.Get(a);
      const KeyType valueB = this->KeyPortal.Get(b);
      if (valueA < valueB)
      {
        return true;
      }
      else if (valueB < valueA)
      {
        return false;
      }
      else
      {
        return a < b;
      }
    }
  };

  // Allows you to pass an IndirectSortPredicate to a device algorithm without knowing the device.
  template <typename KeyArrayType>
  struct IndirectSortPredicateExecObject : public svtkm::cont::ExecutionObjectBase
  {
    const KeyArrayType KeyArray;

    SVTKM_CONT IndirectSortPredicateExecObject(const KeyArrayType& keyArray)
      : KeyArray(keyArray)
    {
    }

    template <typename Device>
    IndirectSortPredicate<typename KeyArrayType::template ExecutionTypes<Device>::PortalConst>
      PrepareForExecution(Device) const
    {
      auto keyPortal = this->KeyArray.PrepareForInput(Device());
      return IndirectSortPredicate<decltype(keyPortal)>(keyPortal);
    }
  };

  // Allows Unique to be called on an array that indexes into KeyPortal.
  template <typename KeyPortalType>
  struct IndirectUniquePredicate
  {
    const KeyPortalType KeyPortal;

    SVTKM_CONT
    IndirectUniquePredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    SVTKM_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      return this->KeyPortal.Get(a) == this->KeyPortal.Get(b);
    }
  };

  // Allows you to pass an IndirectUniquePredicate to a device algorithm without knowing the device.
  template <typename KeyArrayType>
  struct IndirectUniquePredicateExecObject : public svtkm::cont::ExecutionObjectBase
  {
    const KeyArrayType KeyArray;

    SVTKM_CONT IndirectUniquePredicateExecObject(const KeyArrayType& keyArray)
      : KeyArray(keyArray)
    {
    }

    template <typename Device>
    IndirectUniquePredicate<typename KeyArrayType::template ExecutionTypes<Device>::PortalConst>
      PrepareForExecution(Device) const
    {
      auto keyPortal = this->KeyArray.PrepareForInput(Device());
      return IndirectUniquePredicate<decltype(keyPortal)>(keyPortal);
    }
  };

  /// Permutes the @a indices array so that it will map @a keys into a stable
  /// sorted order. The @a keys array is not modified.
  ///
  /// @param device The Id for the device on which to compute the sort
  /// @param keys The ArrayHandle containing data to be sorted.
  /// @param indices The ArrayHandle<svtkm::Id> containing the permutation indices.
  ///
  /// @note @a indices is expected to contain the values (0, numKeys] in
  /// increasing order. If the values in @a indices are not sequential, the sort
  /// will succeed and be consistently reproducible, but the result is not
  /// guaranteed to be stable with respect to the original ordering of @a keys.
  template <typename KeyType, typename Storage>
  SVTKM_CONT static void Sort(svtkm::cont::DeviceAdapterId device,
                             const svtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                             IndexArrayType& indices)
  {
    using KeyArrayType = svtkm::cont::ArrayHandle<KeyType, Storage>;
    using SortPredicate = IndirectSortPredicateExecObject<KeyArrayType>;

    SVTKM_ASSERT(keys.GetNumberOfValues() == indices.GetNumberOfValues());

    svtkm::cont::Algorithm::Sort(device, indices, SortPredicate(keys));
  }

  /// Permutes the @a indices array so that it will map @a keys into a stable
  /// sorted order. The @a keys array is not modified.
  ///
  /// @param keys The ArrayHandle containing data to be sorted.
  /// @param indices The ArrayHandle<svtkm::Id> containing the permutation indices.
  ///
  /// @note @a indices is expected to contain the values (0, numKeys] in
  /// increasing order. If the values in @a indices are not sequential, the sort
  /// will succeed and be consistently reproducible, but the result is not
  /// guaranteed to be stable with respect to the original ordering of @a keys.
  template <typename KeyType, typename Storage>
  SVTKM_CONT static void Sort(const svtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                             IndexArrayType& indices)
  {
    StableSortIndices::Sort(svtkm::cont::DeviceAdapterTagAny(), keys, indices);
  }

  /// Returns an index array that maps the @a keys array into a stable sorted
  /// ordering. The @a keys array is not modified.
  ///
  /// This is a convenience overload that generates the index array.
  template <typename KeyType, typename Storage>
  SVTKM_CONT static IndexArrayType Sort(svtkm::cont::DeviceAdapterId device,
                                       const svtkm::cont::ArrayHandle<KeyType, Storage>& keys)
  {
    // Generate the initial index array
    IndexArrayType indices;
    {
      svtkm::cont::ArrayHandleIndex indicesSrc(keys.GetNumberOfValues());
      svtkm::cont::Algorithm::Copy(device, indicesSrc, indices);
    }

    StableSortIndices::Sort(device, keys, indices);

    return indices;
  }

  /// Returns an index array that maps the @a keys array into a stable sorted
  /// ordering. The @a keys array is not modified.
  ///
  /// This is a convenience overload that generates the index array.
  template <typename KeyType, typename Storage>
  SVTKM_CONT static IndexArrayType Sort(const svtkm::cont::ArrayHandle<KeyType, Storage>& keys)
  {
    return StableSortIndices::Sort(svtkm::cont::DeviceAdapterTagAny(), keys);
  }

  /// Reduces the array returned by @a Sort so that the mapped @a keys are
  /// unique. The @a indices array will be modified in-place and the @a keys
  /// array is not modified.
  ///
  template <typename KeyType, typename Storage>
  SVTKM_CONT static void Unique(svtkm::cont::DeviceAdapterId device,
                               const svtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                               IndexArrayType& indices)
  {
    using KeyArrayType = svtkm::cont::ArrayHandle<KeyType, Storage>;
    using UniquePredicate = IndirectUniquePredicateExecObject<KeyArrayType>;

    svtkm::cont::Algorithm::Unique(device, indices, UniquePredicate(keys));
  }

  /// Reduces the array returned by @a Sort so that the mapped @a keys are
  /// unique. The @a indices array will be modified in-place and the @a keys
  /// array is not modified.
  ///
  template <typename KeyType, typename Storage>
  SVTKM_CONT static void Unique(const svtkm::cont::ArrayHandle<KeyType, Storage>& keys,
                               IndexArrayType& indices)
  {
    StableSortIndices::Unique(svtkm::cont::DeviceAdapterTagAny(), keys, indices);
  }
};
}
} // end namespace svtkm::worklet

#endif // svtk_m_worklet_SortAndUniqueIndices_h
