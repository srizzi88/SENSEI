//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandlePermutation_h
#define svtk_m_cont_ArrayHandlePermutation_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>

namespace svtkm
{
namespace exec
{
namespace internal
{

template <typename IndexPortalType, typename ValuePortalType>
class SVTKM_ALWAYS_EXPORT ArrayPortalPermutation
{
  using Writable = svtkm::internal::PortalSupportsSets<ValuePortalType>;

public:
  using ValueType = typename ValuePortalType::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalPermutation()
    : IndexPortal()
    , ValuePortal()
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalPermutation(const IndexPortalType& indexPortal, const ValuePortalType& valuePortal)
    : IndexPortal(indexPortal)
    , ValuePortal(valuePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherIP, typename OtherVP>
  SVTKM_EXEC_CONT ArrayPortalPermutation(const ArrayPortalPermutation<OtherIP, OtherVP>& src)
    : IndexPortal(src.GetIndexPortal())
    , ValuePortal(src.GetValuePortal())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->IndexPortal.GetNumberOfValues(); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  ValueType Get(svtkm::Id index) const
  {
    svtkm::Id permutedIndex = this->IndexPortal.Get(index);
    return this->ValuePortal.Get(permutedIndex);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC void Set(svtkm::Id index, const ValueType& value) const
  {
    svtkm::Id permutedIndex = this->IndexPortal.Get(index);
    this->ValuePortal.Set(permutedIndex, value);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const IndexPortalType& GetIndexPortal() const { return this->IndexPortal; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const ValuePortalType& GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};
}
}
} // namespace svtkm::exec::internal

namespace svtkm
{
namespace cont
{

template <typename IndexStorageTag, typename ValueStorageTag>
struct SVTKM_ALWAYS_EXPORT StorageTagPermutation
{
};

namespace internal
{

template <typename T, typename IndexStorageTag, typename ValueStorageTag>
class Storage<T, svtkm::cont::StorageTagPermutation<IndexStorageTag, ValueStorageTag>>
{
  SVTKM_STATIC_ASSERT_MSG(
    (svtkm::cont::internal::IsValidArrayHandle<svtkm::Id, IndexStorageTag>::value),
    "Invalid index storage tag.");
  SVTKM_STATIC_ASSERT_MSG((svtkm::cont::internal::IsValidArrayHandle<T, ValueStorageTag>::value),
                         "Invalid value storage tag.");

public:
  using IndexArrayType = svtkm::cont::ArrayHandle<svtkm::Id, IndexStorageTag>;
  using ValueArrayType = svtkm::cont::ArrayHandle<T, ValueStorageTag>;

  using ValueType = T;

  using PortalType =
    svtkm::exec::internal::ArrayPortalPermutation<typename IndexArrayType::PortalConstControl,
                                                 typename ValueArrayType::PortalControl>;
  using PortalConstType =
    svtkm::exec::internal::ArrayPortalPermutation<typename IndexArrayType::PortalConstControl,
                                                 typename ValueArrayType::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const IndexArrayType& indexArray, const ValueArrayType& valueArray)
    : IndexArray(indexArray)
    , ValueArray(valueArray)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->IndexArray.GetPortalConstControl(),
                      this->ValueArray.GetPortalControl());
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(this->IndexArray.GetPortalConstControl(),
                           this->ValueArray.GetPortalConstControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->IndexArray.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandlePermutation cannot be allocated.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandlePermutation cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  SVTKM_CONT
  const IndexArrayType& GetIndexArray() const { return this->IndexArray; }

  SVTKM_CONT
  const ValueArrayType& GetValueArray() const { return this->ValueArray; }

private:
  IndexArrayType IndexArray;
  ValueArrayType ValueArray;
  bool Valid;
};

template <typename T, typename IndexStorageTag, typename ValueStorageTag, typename Device>
class ArrayTransfer<T, StorageTagPermutation<IndexStorageTag, ValueStorageTag>, Device>
{
public:
  using ValueType = T;

private:
  using StorageTag = StorageTagPermutation<IndexStorageTag, ValueStorageTag>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

  using IndexArrayType = typename StorageType::IndexArrayType;
  using ValueArrayType = typename StorageType::ValueArrayType;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::exec::internal::ArrayPortalPermutation<
    typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
    typename ValueArrayType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution = svtkm::exec::internal::ArrayPortalPermutation<
    typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
    typename ValueArrayType::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : IndexArray(storage->GetIndexArray())
    , ValueArray(storage->GetValueArray())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->IndexArray.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->IndexArray.PrepareForInput(Device()),
                                this->ValueArray.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->IndexArray.PrepareForInput(Device()),
                           this->ValueArray.PrepareForInPlace(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    if (numberOfValues != this->GetNumberOfValues())
    {
      throw svtkm::cont::ErrorBadValue(
        "An ArrayHandlePermutation can be used as an output array, "
        "but it cannot be resized. Make sure the index array is sized "
        "to the appropriate length before trying to prepare for output.");
    }

    // We cannot practically allocate ValueArray because we do not know the
    // range of indices. We try to check by seeing if ValueArray has no
    // entries, which clearly indicates that it is not allocated. Otherwise,
    // we have to assume the allocation is correct.
    if ((numberOfValues > 0) && (this->ValueArray.GetNumberOfValues() < 1))
    {
      throw svtkm::cont::ErrorBadValue(
        "The value array must be pre-allocated before it is used for the "
        "output of ArrayHandlePermutation.");
    }

    return PortalExecution(
      this->IndexArray.PrepareForInput(Device()),
      this->ValueArray.PrepareForOutput(this->ValueArray.GetNumberOfValues(), Device()));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandlePermutation cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    this->IndexArray.ReleaseResourcesExecution();
    this->ValueArray.ReleaseResourcesExecution();
  }

private:
  IndexArrayType IndexArray;
  ValueArrayType ValueArray;
};

} // namespace internal

/// \brief Implicitly permutes the values in an array.
///
/// ArrayHandlePermutation is a specialization of ArrayHandle. It takes two
/// delegate array handles: an array of indices and an array of values. The
/// array handle created contains the values given permuted by the indices
/// given. So for a given index i, ArrayHandlePermutation looks up the i-th
/// value in the index array to get permuted index j and then gets the j-th
/// value in the value array. This index permutation is done on the fly rather
/// than creating a copy of the array.
///
/// An ArrayHandlePermutation can be used for either input or output. However,
/// if used for output the array must be pre-allocated. That is, the indices
/// must already be established and the values must have an allocation large
/// enough to accommodate the indices. An output ArrayHandlePermutation will
/// only have values changed. The indices are never changed.
///
/// When using ArrayHandlePermutation great care should be taken to make sure
/// that every index in the index array points to a valid position in the value
/// array. Otherwise, access validations will occur. Also, be wary of duplicate
/// indices that point to the same location in the value array. For input
/// arrays, this is fine. However, this could result in unexpected results for
/// using as output and is almost certainly wrong for using as in-place.
///
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
class ArrayHandlePermutation
  : public svtkm::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      svtkm::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                        typename ValueArrayHandleType::StorageTag>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  SVTKM_IS_ARRAY_HANDLE(IndexArrayHandleType);
  SVTKM_IS_ARRAY_HANDLE(ValueArrayHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandlePermutation,
    (ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>),
    (svtkm::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      svtkm::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                        typename ValueArrayHandleType::StorageTag>>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandlePermutation(const IndexArrayHandleType& indexArray,
                         const ValueArrayHandleType& valueArray)
    : Superclass(StorageType(indexArray, valueArray))
  {
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.

template <typename IndexArrayHandleType, typename ValueArrayHandleType>
SVTKM_CONT svtkm::cont::ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>
make_ArrayHandlePermutation(IndexArrayHandleType indexArray, ValueArrayHandleType valueArray)
{
  return ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>(indexArray, valueArray);
}
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename IdxAH, typename ValAH>
struct SerializableTypeString<svtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Permutation<" + SerializableTypeString<IdxAH>::Get() + "," +
      SerializableTypeString<ValAH>::Get() + ">";
    return name;
  }
};

template <typename T, typename IdxST, typename ValST>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagPermutation<IdxST, ValST>>>
  : SerializableTypeString<
      svtkm::cont::ArrayHandlePermutation<svtkm::cont::ArrayHandle<svtkm::Id, IdxST>,
                                         svtkm::cont::ArrayHandle<T, ValST>>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename IdxAH, typename ValAH>
struct Serialization<svtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
private:
  using Type = svtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetIndexArray());
    svtkmdiy::save(bb, storage.GetValueArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    IdxAH indices;
    ValAH values;

    svtkmdiy::load(bb, indices);
    svtkmdiy::load(bb, values);

    obj = svtkm::cont::make_ArrayHandlePermutation(indices, values);
  }
};

template <typename T, typename IdxST, typename ValST>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagPermutation<IdxST, ValST>>>
  : Serialization<svtkm::cont::ArrayHandlePermutation<svtkm::cont::ArrayHandle<svtkm::Id, IdxST>,
                                                     svtkm::cont::ArrayHandle<T, ValST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandlePermutation_h
