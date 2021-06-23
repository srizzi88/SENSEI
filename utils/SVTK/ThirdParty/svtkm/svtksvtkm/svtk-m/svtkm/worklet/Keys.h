//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Keys_h
#define svtk_m_worklet_Keys_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/Logging.h>

#include <svtkm/Hash.h>

#include <svtkm/exec/internal/ReduceByKeyLookup.h>

#include <svtkm/cont/arg/TransportTagKeyedValuesIn.h>
#include <svtkm/cont/arg/TransportTagKeyedValuesInOut.h>
#include <svtkm/cont/arg/TransportTagKeyedValuesOut.h>
#include <svtkm/cont/arg/TransportTagKeysIn.h>
#include <svtkm/cont/arg/TypeCheckTagKeys.h>

#include <svtkm/worklet/StableSortIndices.h>
#include <svtkm/worklet/svtkm_worklet_export.h>

#include <svtkm/BinaryOperators.h>

namespace svtkm
{
namespace worklet
{

/// Select the type of sort for BuildArrays calls. Unstable sorting is faster
/// but will not produce consistent ordering for equal keys. Stable sorting
/// is slower, but keeps equal keys in their original order.
enum class KeysSortType
{
  Unstable = 0,
  Stable = 1
};

/// \brief Manage keys for a \c WorkletReduceByKey.
///
/// The \c WorkletReduceByKey worklet (and its associated \c
/// DispatcherReduceByKey) take an array of keys for its input domain, find all
/// identical keys, and runs a worklet that produces a single value for every
/// key given all matching values. This class is used as the associated input
/// for the keys input domain.
///
/// \c Keys is templated on the key array handle type and accepts an instance
/// of this array handle as its constructor. It builds the internal structures
/// needed to use the keys.
///
/// The same \c Keys structure can be used for multiple different \c Invoke of
/// different dispatchers. When used in this way, the processing done in the \c
/// Keys structure is reused for all the \c Invoke. This is more efficient than
/// creating a different \c Keys structure for each \c Invoke.
///
template <typename T>
class SVTKM_ALWAYS_EXPORT Keys
{
public:
  using KeyType = T;
  using KeyArrayHandleType = svtkm::cont::ArrayHandle<KeyType>;

  SVTKM_CONT
  Keys();

  /// \b Construct a Keys class from an array of keys.
  ///
  /// Given an array of keys, construct a \c Keys class that will manage
  /// using these keys to perform reduce-by-key operations.
  ///
  /// The input keys object is not modified and the result is not stable
  /// sorted. This is the equivalent of calling
  /// `BuildArrays(keys, KeysSortType::Unstable, device)`.
  ///
  template <typename KeyStorage>
  SVTKM_CONT Keys(const svtkm::cont::ArrayHandle<KeyType, KeyStorage>& keys,
                 svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny())
  {
    this->BuildArrays(keys, KeysSortType::Unstable, device);
  }

  /// Build the internal arrays without modifying the input. This is more
  /// efficient for stable sorted arrays, but requires an extra copy of the
  /// keys for unstable sorting.
  template <typename KeyArrayType>
  SVTKM_CONT void BuildArrays(
    const KeyArrayType& keys,
    KeysSortType sort,
    svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

  /// Build the internal arrays and also sort the input keys. This is more
  /// efficient for unstable sorting, but requires an extra copy for stable
  /// sorting.
  template <typename KeyArrayType>
  SVTKM_CONT void BuildArraysInPlace(
    KeyArrayType& keys,
    KeysSortType sort,
    svtkm::cont::DeviceAdapterId device = svtkm::cont::DeviceAdapterTagAny());

  SVTKM_CONT
  svtkm::Id GetInputRange() const { return this->UniqueKeys.GetNumberOfValues(); }

  SVTKM_CONT
  KeyArrayHandleType GetUniqueKeys() const { return this->UniqueKeys; }

  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::Id> GetSortedValuesMap() const { return this->SortedValuesMap; }

  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::Id> GetOffsets() const { return this->Offsets; }

  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::IdComponent> GetCounts() const { return this->Counts; }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->SortedValuesMap.GetNumberOfValues(); }

  template <typename Device>
  struct ExecutionTypes
  {
    using KeyPortal = typename KeyArrayHandleType::template ExecutionTypes<Device>::PortalConst;
    using IdPortal =
      typename svtkm::cont::ArrayHandle<svtkm::Id>::template ExecutionTypes<Device>::PortalConst;
    using IdComponentPortal = typename svtkm::cont::ArrayHandle<
      svtkm::IdComponent>::template ExecutionTypes<Device>::PortalConst;

    using Lookup = svtkm::exec::internal::ReduceByKeyLookup<KeyPortal, IdPortal, IdComponentPortal>;
  };

  template <typename Device>
  SVTKM_CONT typename ExecutionTypes<Device>::Lookup PrepareForInput(Device) const
  {
    return typename ExecutionTypes<Device>::Lookup(this->UniqueKeys.PrepareForInput(Device()),
                                                   this->SortedValuesMap.PrepareForInput(Device()),
                                                   this->Offsets.PrepareForInput(Device()),
                                                   this->Counts.PrepareForInput(Device()));
  }

  SVTKM_CONT
  bool operator==(const svtkm::worklet::Keys<KeyType>& other) const
  {
    return ((this->UniqueKeys == other.UniqueKeys) &&
            (this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets) &&
            (this->Counts == other.Counts));
  }

  SVTKM_CONT
  bool operator!=(const svtkm::worklet::Keys<KeyType>& other) const { return !(*this == other); }

private:
  KeyArrayHandleType UniqueKeys;
  svtkm::cont::ArrayHandle<svtkm::Id> SortedValuesMap;
  svtkm::cont::ArrayHandle<svtkm::Id> Offsets;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> Counts;

  template <typename KeyArrayType>
  SVTKM_CONT void BuildArraysInternal(KeyArrayType& keys, svtkm::cont::DeviceAdapterId device);

  template <typename KeyArrayType>
  SVTKM_CONT void BuildArraysInternalStable(const KeyArrayType& keys,
                                           svtkm::cont::DeviceAdapterId device);
};

template <typename T>
SVTKM_CONT Keys<T>::Keys() = default;
}
} // namespace svtkm::worklet

// Here we implement the type checks and transports that rely on the Keys
// class. We implement them here because the Keys class is not accessible to
// the arg classes. (The worklet package depends on the cont and exec packages,
// not the other way around.)

namespace svtkm
{
namespace cont
{
namespace arg
{

template <typename KeyType>
struct TypeCheck<svtkm::cont::arg::TypeCheckTagKeys, svtkm::worklet::Keys<KeyType>>
{
  static constexpr bool value = true;
};

template <typename KeyType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagKeysIn, svtkm::worklet::Keys<KeyType>, Device>
{
  using ContObjectType = svtkm::worklet::Keys<KeyType>;
  using ExecObjectType = typename ContObjectType::template ExecutionTypes<Device>::Lookup;

  SVTKM_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const ContObjectType& inputDomain,
                            svtkm::Id,
                            svtkm::Id) const
  {
    if (object != inputDomain)
    {
      throw svtkm::cont::ErrorBadValue("A Keys object must be the input domain.");
    }

    return object.PrepareForInput(Device());
  }

  // If you get a compile error here, it means that you have used a KeysIn
  // tag in your ControlSignature that was not marked as the InputDomain.
  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(const ContObjectType&, const InputDomainType&, svtkm::Id, svtkm::Id) const = delete;
};

template <typename ArrayHandleType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagKeyedValuesIn, ArrayHandleType, Device>
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using PermutedArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = svtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::PortalConst;

  template <typename KeyType>
  SVTKM_CONT ExecObjectType operator()(const ContObjectType& object,
                                      const svtkm::worklet::Keys<KeyType>& keys,
                                      svtkm::Id,
                                      svtkm::Id) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw svtkm::cont::ErrorBadValue("Input values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInput(Device());
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagKeyedValuesInOut, ArrayHandleType, Device>
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using PermutedArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = svtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::Portal;

  template <typename KeyType>
  SVTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const svtkm::worklet::Keys<KeyType>& keys,
                                      svtkm::Id,
                                      svtkm::Id) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw svtkm::cont::ErrorBadValue("Input/output values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInPlace(Device());
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagKeyedValuesOut, ArrayHandleType, Device>
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using PermutedArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType = svtkm::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::template ExecutionTypes<Device>::Portal;

  template <typename KeyType>
  SVTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const svtkm::worklet::Keys<KeyType>& keys,
                                      svtkm::Id,
                                      svtkm::Id) const
  {
    // The PrepareForOutput for ArrayHandleGroupVecVariable and
    // ArrayHandlePermutation cannot determine the actual size expected for the
    // target array (object), so we have to make sure it gets allocated here.
    object.PrepareForOutput(keys.GetNumberOfValues(), Device());

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForOutput(keys.GetInputRange(), Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#ifndef svtk_m_worklet_Keys_cxx

#define SVTK_M_KEYS_EXPORT(T)                                                                       \
  extern template class SVTKM_WORKLET_TEMPLATE_EXPORT svtkm::worklet::Keys<T>;                       \
  extern template SVTKM_WORKLET_TEMPLATE_EXPORT SVTKM_CONT void svtkm::worklet::Keys<T>::BuildArrays( \
    const svtkm::cont::ArrayHandle<T>& keys,                                                        \
    svtkm::worklet::KeysSortType sort,                                                              \
    svtkm::cont::DeviceAdapterId device);                                                           \
  extern template SVTKM_WORKLET_TEMPLATE_EXPORT SVTKM_CONT void svtkm::worklet::Keys<T>::BuildArrays( \
    const svtkm::cont::ArrayHandleVirtual<T>& keys,                                                 \
    svtkm::worklet::KeysSortType sort,                                                              \
    svtkm::cont::DeviceAdapterId device)

SVTK_M_KEYS_EXPORT(svtkm::UInt8);
SVTK_M_KEYS_EXPORT(svtkm::HashType);
SVTK_M_KEYS_EXPORT(svtkm::Id);
SVTK_M_KEYS_EXPORT(svtkm::Id2);
SVTK_M_KEYS_EXPORT(svtkm::Id3);
using Pair_UInt8_Id2 = svtkm::Pair<svtkm::UInt8, svtkm::Id2>;
SVTK_M_KEYS_EXPORT(Pair_UInt8_Id2);
#ifdef SVTKM_USE_64BIT_IDS
SVTK_M_KEYS_EXPORT(svtkm::IdComponent);
#endif

#undef SVTK_M_KEYS_EXPORT

#endif // !svtk_m_worklet_Keys_cxx

#endif //svtk_m_worklet_Keys_h
