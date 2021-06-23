//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleConcatenate_h
#define svtk_m_cont_ArrayHandleConcatenate_h

#include <svtkm/Deprecated.h>
#include <svtkm/StaticAssert.h>

#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename PortalType1, typename PortalType2>
class SVTKM_ALWAYS_EXPORT ArrayPortalConcatenate
{
  using WritableP1 = svtkm::internal::PortalSupportsSets<PortalType1>;
  using WritableP2 = svtkm::internal::PortalSupportsSets<PortalType2>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = typename PortalType1::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalConcatenate()
    : portal1()
    , portal2()
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalConcatenate(const PortalType1& p1, const PortalType2& p2)
    : portal1(p1)
    , portal2(p2)
  {
  }

  // Copy constructor
  template <typename OtherP1, typename OtherP2>
  SVTKM_EXEC_CONT ArrayPortalConcatenate(const ArrayPortalConcatenate<OtherP1, OtherP2>& src)
    : portal1(src.GetPortal1())
    , portal2(src.GetPortal2())
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->portal1.GetNumberOfValues() + this->portal2.GetNumberOfValues();
  }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      return this->portal1.Get(index);
    }
    else
    {
      return this->portal2.Get(index - this->portal1.GetNumberOfValues());
    }
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      this->portal1.Set(index, value);
    }
    else
    {
      this->portal2.Set(index - this->portal1.GetNumberOfValues(), value);
    }
  }

  SVTKM_EXEC_CONT
  const PortalType1& GetPortal1() const { return this->portal1; }

  SVTKM_EXEC_CONT
  const PortalType2& GetPortal2() const { return this->portal2; }

private:
  PortalType1 portal1;
  PortalType2 portal2;
}; // class ArrayPortalConcatenate

} // namespace internal

template <typename StorageTag1, typename StorageTag2>
class SVTKM_ALWAYS_EXPORT StorageTagConcatenate
{
};

namespace internal
{

namespace detail
{

template <typename T, typename ArrayOrStorage, bool IsArrayType>
struct ConcatinateTypeArgImpl;

template <typename T, typename Storage>
struct ConcatinateTypeArgImpl<T, Storage, false>
{
  using StorageTag = Storage;
  using ArrayHandle = svtkm::cont::ArrayHandle<T, StorageTag>;
};

template <typename T, typename Array>
struct ConcatinateTypeArgImpl<T, Array, true>
{
  SVTKM_STATIC_ASSERT_MSG((std::is_same<T, typename Array::ValueType>::value),
                         "Used array with wrong type in ArrayHandleConcatinate.");
  using StorageTag SVTKM_DEPRECATED(
    1.6,
    "Use storage tags instead of array handles in StorageTagConcatenate.") =
    typename Array::StorageTag;
  using ArrayHandle SVTKM_DEPRECATED(
    1.6,
    "Use storage tags instead of array handles in StorageTagConcatenate.") =
    svtkm::cont::ArrayHandle<T, typename Array::StorageTag>;
};

template <typename T, typename ArrayOrStorage>
struct ConcatinateTypeArg
  : ConcatinateTypeArgImpl<T,
                           ArrayOrStorage,
                           svtkm::cont::internal::ArrayHandleCheck<ArrayOrStorage>::type::value>
{
};

} // namespace detail

template <typename T, typename ST1, typename ST2>
class Storage<T, StorageTagConcatenate<ST1, ST2>>
{
  using ArrayHandleType1 = typename detail::ConcatinateTypeArg<T, ST1>::ArrayHandle;
  using ArrayHandleType2 = typename detail::ConcatinateTypeArg<T, ST2>::ArrayHandle;

public:
  using ValueType = T;
  using PortalType = ArrayPortalConcatenate<typename ArrayHandleType1::PortalControl,
                                            typename ArrayHandleType2::PortalControl>;
  using PortalConstType = ArrayPortalConcatenate<typename ArrayHandleType1::PortalConstControl,
                                                 typename ArrayHandleType2::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType1& a1, const ArrayHandleType2& a2)
    : array1(a1)
    , array2(a2)
    , valid(true)
  {
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->valid);
    return PortalConstType(this->array1.GetPortalConstControl(),
                           this->array2.GetPortalConstControl());
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->valid);
    return PortalType(this->array1.GetPortalControl(), this->array2.GetPortalControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->valid);
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorInternal("ArrayHandleConcatenate should not be allocated explicitly. ");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->valid);
    if (numberOfValues < this->array1.GetNumberOfValues())
    {
      this->array1.Shrink(numberOfValues);
      this->array2.Shrink(0);
    }
    else
      this->array2.Shrink(numberOfValues - this->array1.GetNumberOfValues());
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->valid);
    this->array1.ReleaseResources();
    this->array2.ReleaseResources();
  }

  SVTKM_CONT
  const ArrayHandleType1& GetArray1() const
  {
    SVTKM_ASSERT(this->valid);
    return this->array1;
  }

  SVTKM_CONT
  const ArrayHandleType2& GetArray2() const
  {
    SVTKM_ASSERT(this->valid);
    return this->array2;
  }

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2;
  bool valid;
}; // class Storage

template <typename T, typename ST1, typename ST2, typename Device>
class ArrayTransfer<T, StorageTagConcatenate<ST1, ST2>, Device>
{
  using ArrayHandleType1 = typename detail::ConcatinateTypeArg<T, ST1>::ArrayHandle;
  using ArrayHandleType2 = typename detail::ConcatinateTypeArg<T, ST2>::ArrayHandle;
  using StorageTag1 = typename detail::ConcatinateTypeArg<T, ST1>::StorageTag;
  using StorageTag2 = typename detail::ConcatinateTypeArg<T, ST2>::StorageTag;

public:
  using ValueType = T;

private:
  using StorageTag = StorageTagConcatenate<StorageTag1, StorageTag2>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalConcatenate<typename ArrayHandleType1::template ExecutionTypes<Device>::Portal,
                           typename ArrayHandleType2::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalConcatenate<typename ArrayHandleType1::template ExecutionTypes<Device>::PortalConst,
                           typename ArrayHandleType2::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : array1(storage->GetArray1())
    , array2(storage->GetArray2())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->array1.PrepareForInput(Device()),
                                this->array2.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->array1.PrepareForInPlace(Device()),
                           this->array2.PrepareForInPlace(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorInternal("ArrayHandleConcatenate is derived and read-only. ");
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // not need to implement
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    if (numberOfValues < this->array1.GetNumberOfValues())
    {
      this->array1.Shrink(numberOfValues);
      this->array2.Shrink(0);
    }
    else
      this->array2.Shrink(numberOfValues - this->array1.GetNumberOfValues());
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    this->array1.ReleaseResourcesExecution();
    this->array2.ReleaseResourcesExecution();
  }

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2;
};
}
}
} // namespace svtkm::cont::internal

namespace svtkm
{
namespace cont
{

template <typename ArrayHandleType1, typename ArrayHandleType2>
class ArrayHandleConcatenate
  : public svtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                                   StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                         typename ArrayHandleType2::StorageTag>>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleConcatenate,
    (ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>),
    (svtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                             StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                   typename ArrayHandleType2::StorageTag>>));

protected:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleConcatenate(const ArrayHandleType1& array1, const ArrayHandleType2& array2)
    : Superclass(StorageType(array1, array2))
  {
  }
};

template <typename ArrayHandleType1, typename ArrayHandleType2>
SVTKM_CONT ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2> make_ArrayHandleConcatenate(
  const ArrayHandleType1& array1,
  const ArrayHandleType2& array2)
{
  return ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>(array1, array2);
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

template <typename AH1, typename AH2>
struct SerializableTypeString<svtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Concatenate<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleConcatenate<
      typename internal::detail::ConcatinateTypeArg<T, ST1>::ArrayHandle,
      typename internal::detail::ConcatinateTypeArg<T, ST2>::ArrayHandle>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<svtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
private:
  using Type = svtkm::cont::ArrayHandleConcatenate<AH1, AH2>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetArray1());
    svtkmdiy::save(bb, storage.GetArray2());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;

    svtkmdiy::load(bb, array1);
    svtkmdiy::load(bb, array2);

    obj = svtkm::cont::make_ArrayHandleConcatenate(array1, array2);
  }
};

template <typename T, typename ST1, typename ST2>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : Serialization<svtkm::cont::ArrayHandleConcatenate<
      typename svtkm::cont::internal::detail::ConcatinateTypeArg<T, ST1>::ArrayHandle,
      typename svtkm::cont::internal::detail::ConcatinateTypeArg<T, ST2>::ArrayHandle>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleConcatenate_h
