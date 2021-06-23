//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleCartesianProduct_h
#define svtk_m_cont_ArrayHandleCartesianProduct_h

#include <svtkm/Assert.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadAllocation.h>

namespace svtkm
{
namespace exec
{
namespace internal
{

/// \brief An array portal that acts as a 3D cartesian product of 3 arrays.
///
template <typename ValueType_,
          typename PortalTypeFirst_,
          typename PortalTypeSecond_,
          typename PortalTypeThird_>
class SVTKM_ALWAYS_EXPORT ArrayPortalCartesianProduct
{
public:
  using ValueType = ValueType_;
  using IteratorType = ValueType_;
  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;
  using PortalTypeThird = PortalTypeThird_;

  using set_supported_p1 = svtkm::internal::PortalSupportsSets<PortalTypeFirst>;
  using set_supported_p2 = svtkm::internal::PortalSupportsSets<PortalTypeSecond>;
  using set_supported_p3 = svtkm::internal::PortalSupportsSets<PortalTypeThird>;

  using Writable = std::integral_constant<bool,
                                          set_supported_p1::value && set_supported_p2::value &&
                                            set_supported_p3::value>;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalCartesianProduct()
    : PortalFirst()
    , PortalSecond()
    , PortalThird()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  SVTKM_CONT
  ArrayPortalCartesianProduct(const PortalTypeFirst& portalfirst,
                              const PortalTypeSecond& portalsecond,
                              const PortalTypeThird& portalthird)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
    , PortalThird(portalthird)
  {
  }

  /// Copy constructor for any other ArrayPortalCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///

  template <class OtherV, class OtherP1, class OtherP2, class OtherP3>
  SVTKM_CONT ArrayPortalCartesianProduct(
    const ArrayPortalCartesianProduct<OtherV, OtherP1, OtherP2, OtherP3>& src)
    : PortalFirst(src.GetPortalFirst())
    , PortalSecond(src.GetPortalSecond())
    , PortalThird(src.GetPortalThird())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->PortalFirst.GetNumberOfValues() * this->PortalSecond.GetNumberOfValues() *
      this->PortalThird.GetNumberOfValues();
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->GetNumberOfValues());

    svtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    svtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    svtkm::Id dim12 = dim1 * dim2;
    svtkm::Id idx12 = index % dim12;
    svtkm::Id i1 = idx12 % dim1;
    svtkm::Id i2 = idx12 / dim1;
    svtkm::Id i3 = index / dim12;

    return svtkm::make_Vec(
      this->PortalFirst.Get(i1), this->PortalSecond.Get(i2), this->PortalThird.Get(i3));
  }


  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->GetNumberOfValues());

    svtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    svtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    svtkm::Id dim12 = dim1 * dim2;
    svtkm::Id idx12 = index % dim12;

    svtkm::Id i1 = idx12 % dim1;
    svtkm::Id i2 = idx12 / dim1;
    svtkm::Id i3 = index / dim12;

    this->PortalFirst.Set(i1, value[0]);
    this->PortalSecond.Set(i2, value[1]);
    this->PortalThird.Set(i3, value[2]);
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const PortalTypeThird& GetThirdPortal() const { return this->PortalThird; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
};
}
}
} // namespace svtkm::exec::internal

namespace svtkm
{
namespace cont
{

template <typename StorageTag1, typename StorageTag2, typename StorageTag3>
struct SVTKM_ALWAYS_EXPORT StorageTagCartesianProduct
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename AH1, typename AH2, typename AH3>
struct ArrayHandleCartesianProductTraits
{
  SVTKM_IS_ARRAY_HANDLE(AH1);
  SVTKM_IS_ARRAY_HANDLE(AH2);
  SVTKM_IS_ARRAY_HANDLE(AH3);

  using ComponentType = typename AH1::ValueType;
  SVTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH2::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");
  SVTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH3::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");

  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType = svtkm::Vec<ComponentType, 3>;

  /// The appropriately templated tag.
  ///
  using Tag = svtkm::cont::StorageTagCartesianProduct<typename AH1::StorageTag,
                                                     typename AH2::StorageTag,
                                                     typename AH3::StorageTag>;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  using Superclass = svtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T, typename ST1, typename ST2, typename ST3>
class Storage<svtkm::Vec<T, 3>, svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  using AH1 = svtkm::cont::ArrayHandle<T, ST1>;
  using AH2 = svtkm::cont::ArrayHandle<T, ST2>;
  using AH3 = svtkm::cont::ArrayHandle<T, ST3>;

public:
  using ValueType = svtkm::Vec<typename AH1::ValueType, 3>;

  using PortalType = svtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                                       typename AH1::PortalControl,
                                                                       typename AH2::PortalControl,
                                                                       typename AH3::PortalControl>;
  using PortalConstType =
    svtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                      typename AH1::PortalConstControl,
                                                      typename AH2::PortalConstControl,
                                                      typename AH3::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : FirstArray()
    , SecondArray()
    , ThirdArray()
  {
  }

  SVTKM_CONT
  Storage(const AH1& array1, const AH2& array2, const AH3& array3)
    : FirstArray(array1)
    , SecondArray(array2)
    , ThirdArray(array3)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->FirstArray.GetPortalControl(),
                      this->SecondArray.GetPortalControl(),
                      this->ThirdArray.GetPortalControl());
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->FirstArray.GetPortalConstControl(),
                           this->SecondArray.GetPortalConstControl(),
                           this->ThirdArray.GetPortalConstControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->FirstArray.GetNumberOfValues() * this->SecondArray.GetNumberOfValues() *
      this->ThirdArray.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id /*numberOfValues*/)
  {
    throw svtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id /*numberOfValues*/)
  {
    throw svtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the arrays, which may be used elsewhere.
  }

  SVTKM_CONT
  const AH1& GetFirstArray() const { return this->FirstArray; }

  SVTKM_CONT
  const AH2& GetSecondArray() const { return this->SecondArray; }

  SVTKM_CONT
  const AH3& GetThirdArray() const { return this->ThirdArray; }

private:
  AH1 FirstArray;
  AH2 SecondArray;
  AH3 ThirdArray;
};

template <typename T, typename ST1, typename ST2, typename ST3, typename Device>
class ArrayTransfer<svtkm::Vec<T, 3>, svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>, Device>
{
public:
  using ValueType = svtkm::Vec<T, 3>;

private:
  using AH1 = svtkm::cont::ArrayHandle<T, ST1>;
  using AH2 = svtkm::cont::ArrayHandle<T, ST2>;
  using AH3 = svtkm::cont::ArrayHandle<T, ST3>;

  using StorageTag = svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename AH1::template ExecutionTypes<Device>::Portal,
    typename AH2::template ExecutionTypes<Device>::Portal,
    typename AH3::template ExecutionTypes<Device>::Portal>;

  using PortalConstExecution = svtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename AH1::template ExecutionTypes<Device>::PortalConst,
    typename AH2::template ExecutionTypes<Device>::PortalConst,
    typename AH3::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : FirstArray(storage->GetFirstArray())
    , SecondArray(storage->GetSecondArray())
    , ThirdArray(storage->GetThirdArray())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    return this->FirstArray.GetNumberOfValues() * this->SecondArray.GetNumberOfValues() *
      this->ThirdArray.GetNumberOfValues();
  }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()),
                                this->ThirdArray.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadAllocation(
      "Cannot write to an ArrayHandleCartesianProduct. It does not make "
      "sense because there is overlap in the data.");
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadAllocation(
      "Cannot write to an ArrayHandleCartesianProduct. It does not make "
      "sense because there is overlap in the data.");
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id /*numberOfValues*/)
  {
    throw svtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    this->FirstArray.ReleaseResourcesExecution();
    this->SecondArray.ReleaseResourcesExecution();
    this->ThirdArray.ReleaseResourcesExecution();
  }

private:
  AH1 FirstArray;
  AH2 SecondArray;
  AH3 ThirdArray;
};
} // namespace internal

/// ArrayHandleCartesianProduct is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class ArrayHandleCartesianProduct
  : public internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                       SecondHandleType,
                                                       ThirdHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  SVTKM_IS_ARRAY_HANDLE(FirstHandleType);
  SVTKM_IS_ARRAY_HANDLE(SecondHandleType);
  SVTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCartesianProduct,
    (ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>),
    (typename internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                          SecondHandleType,
                                                          ThirdHandleType>::Superclass));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleCartesianProduct(const FirstHandleType& firstArray,
                              const SecondHandleType& secondArray,
                              const ThirdHandleType& thirdArray)
    : Superclass(StorageType(firstArray, secondArray, thirdArray))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCartesianProduct() {}
};

/// A convenience function for creating an ArrayHandleCartesianProduct. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
SVTKM_CONT
  svtkm::cont::ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>
  make_ArrayHandleCartesianProduct(const FirstHandleType& first,
                                   const SecondHandleType& second,
                                   const ThirdHandleType& third)
{
  return ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>(
    first, second, third);
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

template <typename AH1, typename AH2, typename AH3>
struct SerializableTypeString<svtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_CartesianProduct<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + "," + SerializableTypeString<AH3>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<T, ST1>,
                                                                   svtkm::cont::ArrayHandle<T, ST2>,
                                                                   svtkm::cont::ArrayHandle<T, ST3>>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2, typename AH3>
struct Serialization<svtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
private:
  using Type = typename svtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetFirstArray());
    svtkmdiy::save(bb, storage.GetSecondArray());
    svtkmdiy::save(bb, storage.GetThirdArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;
    AH3 array3;

    svtkmdiy::load(bb, array1);
    svtkmdiy::load(bb, array2);
    svtkmdiy::load(bb, array3);

    obj = svtkm::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct Serialization<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, svtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : Serialization<svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<T, ST1>,
                                                          svtkm::cont::ArrayHandle<T, ST2>,
                                                          svtkm::cont::ArrayHandle<T, ST3>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleCartesianProduct_h
