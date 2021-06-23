//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleZip_h
#define svtk_m_cont_ArrayHandleZip_h

#include <svtkm/Pair.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/internal/ArrayPortalHelpers.h>

namespace svtkm
{
namespace exec
{
namespace internal
{

/// \brief An array portal that zips two portals together into a single value
/// for the execution environment
template <typename ValueType_, typename PortalTypeFirst_, typename PortalTypeSecond_>
class ArrayPortalZip
{
  using ReadableP1 = svtkm::internal::PortalSupportsGets<PortalTypeFirst_>;
  using ReadableP2 = svtkm::internal::PortalSupportsGets<PortalTypeSecond_>;
  using WritableP1 = svtkm::internal::PortalSupportsSets<PortalTypeFirst_>;
  using WritableP2 = svtkm::internal::PortalSupportsSets<PortalTypeSecond_>;

  using Readable = std::integral_constant<bool, ReadableP1::value && ReadableP2::value>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = ValueType_;
  using T = typename ValueType::FirstType;
  using U = typename ValueType::SecondType;

  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalZip()
    : PortalFirst()
    , PortalSecond()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  SVTKM_CONT
  ArrayPortalZip(const PortalTypeFirst& portalfirst, const PortalTypeSecond& portalsecond)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
  {
  }

  /// Copy constructor for any other ArrayPortalZip with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV, class OtherF, class OtherS>
  SVTKM_CONT ArrayPortalZip(const ArrayPortalZip<OtherV, OtherF, OtherS>& src)
    : PortalFirst(src.GetFirstPortal())
    , PortalSecond(src.GetSecondPortal())
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->PortalFirst.GetNumberOfValues(); }

  template <typename Readable_ = Readable,
            typename = typename std::enable_if<Readable_::value>::type>
  SVTKM_EXEC_CONT ValueType Get(svtkm::Id index) const noexcept
  {
    return svtkm::make_Pair(this->PortalFirst.Get(index), this->PortalSecond.Get(index));
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const noexcept
  {
    this->PortalFirst.Set(index, value.first);
    this->PortalSecond.Set(index, value.second);
  }

  SVTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  SVTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
};
}
}
} // namespace svtkm::exec::internal

namespace svtkm
{
namespace cont
{

template <typename ST1, typename ST2>
struct SVTKM_ALWAYS_EXPORT StorageTagZip
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename FirstHandleType, typename SecondHandleType>
struct ArrayHandleZipTraits
{
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType =
    svtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>;

  /// The appropriately templated tag.
  ///
  using Tag =
    StorageTagZip<typename FirstHandleType::StorageTag, typename SecondHandleType::StorageTag>;

  /// The superclass for ArrayHandleZip.
  ///
  using Superclass = svtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T1, typename T2, typename ST1, typename ST2>
class Storage<svtkm::Pair<T1, T2>, svtkm::cont::StorageTagZip<ST1, ST2>>
{
  using FirstHandleType = svtkm::cont::ArrayHandle<T1, ST1>;
  using SecondHandleType = svtkm::cont::ArrayHandle<T2, ST2>;

public:
  using ValueType = svtkm::Pair<T1, T2>;

  using PortalType = svtkm::exec::internal::ArrayPortalZip<ValueType,
                                                          typename FirstHandleType::PortalControl,
                                                          typename SecondHandleType::PortalControl>;
  using PortalConstType =
    svtkm::exec::internal::ArrayPortalZip<ValueType,
                                         typename FirstHandleType::PortalConstControl,
                                         typename SecondHandleType::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : FirstArray()
    , SecondArray()
  {
  }

  SVTKM_CONT
  Storage(const FirstHandleType& farray, const SecondHandleType& sarray)
    : FirstArray(farray)
    , SecondArray(sarray)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->FirstArray.GetPortalControl(), this->SecondArray.GetPortalControl());
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->FirstArray.GetPortalConstControl(),
                           this->SecondArray.GetPortalConstControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->FirstArray.GetNumberOfValues() == this->SecondArray.GetNumberOfValues());
    return this->FirstArray.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    this->FirstArray.Allocate(numberOfValues);
    this->SecondArray.Allocate(numberOfValues);
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the two zipped array, which may be used elsewhere.
  }

  SVTKM_CONT
  const FirstHandleType& GetFirstArray() const { return this->FirstArray; }

  SVTKM_CONT
  const SecondHandleType& GetSecondArray() const { return this->SecondArray; }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
};

template <typename T1, typename T2, typename ST1, typename ST2, typename Device>
class ArrayTransfer<svtkm::Pair<T1, T2>, svtkm::cont::StorageTagZip<ST1, ST2>, Device>
{
  using StorageTag = svtkm::cont::StorageTagZip<ST1, ST2>;
  using StorageType = svtkm::cont::internal::Storage<svtkm::Pair<T1, T2>, StorageTag>;

  using FirstHandleType = svtkm::cont::ArrayHandle<T1, ST1>;
  using SecondHandleType = svtkm::cont::ArrayHandle<T2, ST2>;

public:
  using ValueType = svtkm::Pair<T1, T2>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::exec::internal::ArrayPortalZip<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::Portal,
    typename SecondHandleType::template ExecutionTypes<Device>::Portal>;

  using PortalConstExecution = svtkm::exec::internal::ArrayPortalZip<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::PortalConst,
    typename SecondHandleType::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : FirstArray(storage->GetFirstArray())
    , SecondArray(storage->GetSecondArray())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->FirstArray.GetNumberOfValues() == this->SecondArray.GetNumberOfValues());
    return this->FirstArray.GetNumberOfValues();
  }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->FirstArray.PrepareForInPlace(Device()),
                           this->SecondArray.PrepareForInPlace(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(this->FirstArray.PrepareForOutput(numberOfValues, Device()),
                           this->SecondArray.PrepareForOutput(numberOfValues, Device()));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    this->FirstArray.ReleaseResourcesExecution();
    this->SecondArray.ReleaseResourcesExecution();
  }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
};
} // namespace internal

/// ArrayHandleZip is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType>
class ArrayHandleZip
  : public internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  SVTKM_IS_ARRAY_HANDLE(FirstHandleType);

  // If the following line gives a compile error, then the SecondHandleType
  // template argument is not a valid ArrayHandle type.
  SVTKM_IS_ARRAY_HANDLE(SecondHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleZip,
    (ArrayHandleZip<FirstHandleType, SecondHandleType>),
    (typename internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleZip(const FirstHandleType& firstArray, const SecondHandleType& secondArray)
    : Superclass(StorageType(firstArray, secondArray))
  {
  }
};

/// A convenience function for creating an ArrayHandleZip. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType>
SVTKM_CONT svtkm::cont::ArrayHandleZip<FirstHandleType, SecondHandleType> make_ArrayHandleZip(
  const FirstHandleType& first,
  const SecondHandleType& second)
{
  return ArrayHandleZip<FirstHandleType, SecondHandleType>(first, second);
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
struct SerializableTypeString<svtkm::cont::ArrayHandleZip<AH1, AH2>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Zip<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T1, typename T2, typename ST1, typename ST2>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<svtkm::Pair<T1, T2>, svtkm::cont::StorageTagZip<ST1, ST2>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleZip<svtkm::cont::ArrayHandle<T1, ST1>,
                                                      svtkm::cont::ArrayHandle<T2, ST2>>>
{
};
}
} // namespace svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<svtkm::cont::ArrayHandleZip<AH1, AH2>>
{
private:
  using Type = typename svtkm::cont::ArrayHandleZip<AH1, AH2>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetFirstArray());
    svtkmdiy::save(bb, storage.GetSecondArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 a1;
    AH2 a2;

    svtkmdiy::load(bb, a1);
    svtkmdiy::load(bb, a2);

    obj = svtkm::cont::make_ArrayHandleZip(a1, a2);
  }
};

template <typename T1, typename T2, typename ST1, typename ST2>
struct Serialization<
  svtkm::cont::ArrayHandle<svtkm::Pair<T1, T2>, svtkm::cont::StorageTagZip<ST1, ST2>>>
  : Serialization<svtkm::cont::ArrayHandleZip<svtkm::cont::ArrayHandle<T1, ST1>,
                                             svtkm::cont::ArrayHandle<T2, ST2>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleZip_h
