//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_ArrayHandleCompositeVector_h
#define svtk_m_ArrayHandleCompositeVector_h

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/Deprecated.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/VecTraits.h>

#include <svtkmtaotuple/include/Tuple.h>

#include <svtkm/internal/brigand.hpp>

#include <type_traits>

namespace svtkm
{
namespace cont
{
namespace internal
{

namespace compvec
{

// AllAreArrayHandles: ---------------------------------------------------------
// Ensures that all types in ArrayHandlesT... are subclasses of ArrayHandleBase
template <typename... ArrayHandlesT>
struct AllAreArrayHandlesImpl;

template <typename Head, typename... Tail>
struct AllAreArrayHandlesImpl<Head, Tail...>
{
private:
  using Next = AllAreArrayHandlesImpl<Tail...>;
  constexpr static bool HeadValid = std::is_base_of<ArrayHandleBase, Head>::value;

public:
  constexpr static bool Value = HeadValid && Next::Value;
};

template <typename Head>
struct AllAreArrayHandlesImpl<Head>
{
  constexpr static bool Value = std::is_base_of<ArrayHandleBase, Head>::value;
};

template <typename... ArrayHandleTs>
struct AllAreArrayHandles
{
  constexpr static bool Value = AllAreArrayHandlesImpl<ArrayHandleTs...>::Value;
};

// GetValueType: ---------------------------------------------------------------
// Determines the output ValueType of the objects in TupleType, a svtkmstd::tuple
// which can contain ArrayHandles, ArrayPortals...anything with a ValueType
// defined, really. For example, if the input TupleType contains 3 types with
// Float32 ValueTypes, then the ValueType defined here will be Vec<Float32, 3>.
// This also validates that all members have the same ValueType.
template <typename TupleType>
struct GetValueTypeImpl;

template <typename Head, typename... Tail>
struct GetValueTypeImpl<svtkmstd::tuple<Head, Tail...>>
{
  using Type = typename Head::ValueType;

private:
  using Next = GetValueTypeImpl<svtkmstd::tuple<Tail...>>;
  SVTKM_STATIC_ASSERT_MSG(SVTKM_PASS_COMMAS(std::is_same<Type, typename Next::Type>::value),
                         "ArrayHandleCompositeVector must be built from "
                         "ArrayHandles with the same ValueTypes.");
};

template <typename Head>
struct GetValueTypeImpl<svtkmstd::tuple<Head>>
{
  using Type = typename Head::ValueType;
};

template <typename TupleType>
struct GetValueType
{
  SVTKM_STATIC_ASSERT(svtkmstd::tuple_size<TupleType>::value >= 1);

  static const svtkm::IdComponent COUNT =
    static_cast<svtkm::IdComponent>(svtkmstd::tuple_size<TupleType>::value);
  using ComponentType = typename GetValueTypeImpl<TupleType>::Type;
  using ValueType = svtkm::Vec<ComponentType, COUNT>;
};

// TupleTypePrepend: -----------------------------------------------------------
// Prepend a type to a tuple, defining the new tuple in Type.
template <typename PrependType, typename TupleType>
struct TupleTypePrepend;

template <typename PrependType, typename... TupleTypes>
struct TupleTypePrepend<PrependType, svtkmstd::tuple<TupleTypes...>>
{
  using Type = svtkmstd::tuple<PrependType, TupleTypes...>;
};

// ArrayTupleForEach: ----------------------------------------------------------
// Collection of methods that iterate through the arrays in ArrayTuple to
// implement the ArrayHandle API.
template <std::size_t Index, std::size_t Count, typename ArrayTuple>
struct ArrayTupleForEach
{
  using Next = ArrayTupleForEach<Index + 1, Count, ArrayTuple>;

  template <typename PortalTuple>
  SVTKM_CONT static void GetPortalTupleControl(ArrayTuple& arrays, PortalTuple& portals)
  {
    svtkmstd::get<Index>(portals) = svtkmstd::get<Index>(arrays).GetPortalControl();
    Next::GetPortalTupleControl(arrays, portals);
  }

  template <typename PortalTuple>
  SVTKM_CONT static void GetPortalConstTupleControl(const ArrayTuple& arrays, PortalTuple& portals)
  {
    svtkmstd::get<Index>(portals) = svtkmstd::get<Index>(arrays).GetPortalConstControl();
    Next::GetPortalConstTupleControl(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForInput(const ArrayTuple& arrays, PortalTuple& portals)
  {
    svtkmstd::get<Index>(portals) = svtkmstd::get<Index>(arrays).PrepareForInput(DeviceTag());
    Next::template PrepareForInput<DeviceTag>(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForInPlace(ArrayTuple& arrays, PortalTuple& portals)
  {
    svtkmstd::get<Index>(portals) = svtkmstd::get<Index>(arrays).PrepareForInPlace(DeviceTag());
    Next::template PrepareForInPlace<DeviceTag>(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForOutput(ArrayTuple& arrays,
                                         PortalTuple& portals,
                                         svtkm::Id numValues)
  {
    svtkmstd::get<Index>(portals) =
      svtkmstd::get<Index>(arrays).PrepareForOutput(numValues, DeviceTag());
    Next::template PrepareForOutput<DeviceTag>(arrays, portals, numValues);
  }

  SVTKM_CONT
  static void Allocate(ArrayTuple& arrays, svtkm::Id numValues)
  {
    svtkmstd::get<Index>(arrays).Allocate(numValues);
    Next::Allocate(arrays, numValues);
  }

  SVTKM_CONT
  static void Shrink(ArrayTuple& arrays, svtkm::Id numValues)
  {
    svtkmstd::get<Index>(arrays).Shrink(numValues);
    Next::Shrink(arrays, numValues);
  }

  SVTKM_CONT
  static void ReleaseResources(ArrayTuple& arrays)
  {
    svtkmstd::get<Index>(arrays).ReleaseResources();
    Next::ReleaseResources(arrays);
  }
};

template <std::size_t Index, typename ArrayTuple>
struct ArrayTupleForEach<Index, Index, ArrayTuple>
{
  template <typename PortalTuple>
  SVTKM_CONT static void GetPortalTupleControl(ArrayTuple&, PortalTuple&)
  {
  }

  template <typename PortalTuple>
  SVTKM_CONT static void GetPortalConstTupleControl(const ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForInput(const ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForInPlace(ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  SVTKM_CONT static void PrepareForOutput(ArrayTuple&, PortalTuple&, svtkm::Id)
  {
  }

  SVTKM_CONT static void Allocate(ArrayTuple&, svtkm::Id) {}

  SVTKM_CONT static void Shrink(ArrayTuple&, svtkm::Id) {}

  SVTKM_CONT static void ReleaseResources(ArrayTuple&) {}
};

// PortalTupleTraits: ----------------------------------------------------------
// Determine types of ArrayHandleCompositeVector portals and construct the
// portals from the input arrays.
template <typename ArrayTuple>
struct PortalTupleTypeGeneratorImpl;

template <typename Head, typename... Tail>
struct PortalTupleTypeGeneratorImpl<svtkmstd::tuple<Head, Tail...>>
{
  using Next = PortalTupleTypeGeneratorImpl<svtkmstd::tuple<Tail...>>;
  using PortalControlTuple = typename TupleTypePrepend<typename Head::PortalControl,
                                                       typename Next::PortalControlTuple>::Type;
  using PortalConstControlTuple =
    typename TupleTypePrepend<typename Head::PortalConstControl,
                              typename Next::PortalConstControlTuple>::Type;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = typename TupleTypePrepend<
      typename Head::template ExecutionTypes<DeviceTag>::Portal,
      typename Next::template ExecutionTypes<DeviceTag>::PortalTuple>::Type;
    using PortalConstTuple = typename TupleTypePrepend<
      typename Head::template ExecutionTypes<DeviceTag>::PortalConst,
      typename Next::template ExecutionTypes<DeviceTag>::PortalConstTuple>::Type;
  };
};

template <typename Head>
struct PortalTupleTypeGeneratorImpl<svtkmstd::tuple<Head>>
{
  using PortalControlTuple = svtkmstd::tuple<typename Head::PortalControl>;
  using PortalConstControlTuple = svtkmstd::tuple<typename Head::PortalConstControl>;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = svtkmstd::tuple<typename Head::template ExecutionTypes<DeviceTag>::Portal>;
    using PortalConstTuple =
      svtkmstd::tuple<typename Head::template ExecutionTypes<DeviceTag>::PortalConst>;
  };
};

template <typename ArrayTuple>
struct PortalTupleTraits
{
private:
  using TypeGenerator = PortalTupleTypeGeneratorImpl<ArrayTuple>;
  using ForEachArray = ArrayTupleForEach<0, svtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;

public:
  using PortalTuple = typename TypeGenerator::PortalControlTuple;
  using PortalConstTuple = typename TypeGenerator::PortalConstControlTuple;

  SVTKM_STATIC_ASSERT(svtkmstd::tuple_size<ArrayTuple>::value ==
                     svtkmstd::tuple_size<PortalTuple>::value);
  SVTKM_STATIC_ASSERT(svtkmstd::tuple_size<ArrayTuple>::value ==
                     svtkmstd::tuple_size<PortalConstTuple>::value);

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = typename TypeGenerator::template ExecutionTypes<DeviceTag>::PortalTuple;
    using PortalConstTuple =
      typename TypeGenerator::template ExecutionTypes<DeviceTag>::PortalConstTuple;

    SVTKM_STATIC_ASSERT(svtkmstd::tuple_size<ArrayTuple>::value ==
                       svtkmstd::tuple_size<PortalTuple>::value);
    SVTKM_STATIC_ASSERT(svtkmstd::tuple_size<ArrayTuple>::value ==
                       svtkmstd::tuple_size<PortalConstTuple>::value);
  };

  SVTKM_CONT
  static const PortalTuple GetPortalTupleControl(ArrayTuple& arrays)
  {
    PortalTuple portals;
    ForEachArray::GetPortalTupleControl(arrays, portals);
    return portals;
  }

  SVTKM_CONT
  static const PortalConstTuple GetPortalConstTupleControl(const ArrayTuple& arrays)
  {
    PortalConstTuple portals;
    ForEachArray::GetPortalConstTupleControl(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  SVTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalConstTuple PrepareForInput(
    const ArrayTuple& arrays)
  {
    typename ExecutionTypes<DeviceTag>::PortalConstTuple portals;
    ForEachArray::template PrepareForInput<DeviceTag>(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  SVTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalTuple PrepareForInPlace(
    ArrayTuple& arrays)
  {
    typename ExecutionTypes<DeviceTag>::PortalTuple portals;
    ForEachArray::template PrepareForInPlace<DeviceTag>(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  SVTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalTuple PrepareForOutput(
    ArrayTuple& arrays,
    svtkm::Id numValues)
  {
    typename ExecutionTypes<DeviceTag>::PortalTuple portals;
    ForEachArray::template PrepareForOutput<DeviceTag>(arrays, portals, numValues);
    return portals;
  }
};

// ArraySizeValidator: ---------------------------------------------------------
// Call Exec(ArrayTuple, NumValues) to ensure that all arrays in the tuple have
// the specified number of values.
template <std::size_t Index, std::size_t Count, typename TupleType>
struct ArraySizeValidatorImpl
{
  using Next = ArraySizeValidatorImpl<Index + 1, Count, TupleType>;

  SVTKM_CONT
  static bool Exec(const TupleType& tuple, svtkm::Id numVals)
  {
    return svtkmstd::get<Index>(tuple).GetNumberOfValues() == numVals && Next::Exec(tuple, numVals);
  }
};

template <std::size_t Index, typename TupleType>
struct ArraySizeValidatorImpl<Index, Index, TupleType>
{
  SVTKM_CONT
  static bool Exec(const TupleType&, svtkm::Id) { return true; }
};

template <typename TupleType>
struct ArraySizeValidator
{
  SVTKM_CONT
  static bool Exec(const TupleType& tuple, svtkm::Id numVals)
  {
    return ArraySizeValidatorImpl<0, svtkmstd::tuple_size<TupleType>::value, TupleType>::Exec(
      tuple, numVals);
  }
};

template <typename PortalList>
using AllPortalsAreWritable =
  typename brigand::all<PortalList,
                        brigand::bind<svtkm::internal::PortalSupportsSets, brigand::_1>>::type;

} // end namespace compvec

template <typename PortalTuple>
class SVTKM_ALWAYS_EXPORT ArrayPortalCompositeVector
{
  using Writable = compvec::AllPortalsAreWritable<PortalTuple>;

public:
  using ValueType = typename compvec::GetValueType<PortalTuple>::ValueType;

private:
  using Traits = svtkm::VecTraits<ValueType>;

  // Get: ----------------------------------------------------------------------
  template <svtkm::IdComponent VectorIndex, typename PortalTupleT>
  struct GetImpl;

  template <svtkm::IdComponent VectorIndex, typename Head, typename... Tail>
  struct GetImpl<VectorIndex, svtkmstd::tuple<Head, Tail...>>
  {
    using Next = GetImpl<VectorIndex + 1, svtkmstd::tuple<Tail...>>;

    SVTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, ValueType& vec, svtkm::Id arrayIndex)
    {
      Traits::SetComponent(vec, VectorIndex, svtkmstd::get<VectorIndex>(portals).Get(arrayIndex));
      Next::Exec(portals, vec, arrayIndex);
    }
  };

  template <svtkm::IdComponent VectorIndex, typename Head>
  struct GetImpl<VectorIndex, svtkmstd::tuple<Head>>
  {
    SVTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, ValueType& vec, svtkm::Id arrayIndex)
    {
      Traits::SetComponent(vec, VectorIndex, svtkmstd::get<VectorIndex>(portals).Get(arrayIndex));
    }
  };

  // Set: ----------------------------------------------------------------------
  template <svtkm::IdComponent VectorIndex, typename PortalTupleT>
  struct SetImpl;

  template <svtkm::IdComponent VectorIndex, typename Head, typename... Tail>
  struct SetImpl<VectorIndex, svtkmstd::tuple<Head, Tail...>>
  {
    using Next = SetImpl<VectorIndex + 1, svtkmstd::tuple<Tail...>>;

    SVTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, const ValueType& vec, svtkm::Id arrayIndex)
    {
      svtkmstd::get<VectorIndex>(portals).Set(arrayIndex, Traits::GetComponent(vec, VectorIndex));
      Next::Exec(portals, vec, arrayIndex);
    }
  };

  template <svtkm::IdComponent VectorIndex, typename Head>
  struct SetImpl<VectorIndex, svtkmstd::tuple<Head>>
  {
    SVTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, const ValueType& vec, svtkm::Id arrayIndex)
    {
      svtkmstd::get<VectorIndex>(portals).Set(arrayIndex, Traits::GetComponent(vec, VectorIndex));
    }
  };

public:
  SVTKM_EXEC_CONT
  ArrayPortalCompositeVector() {}

  SVTKM_CONT
  ArrayPortalCompositeVector(const PortalTuple& portals)
    : Portals(portals)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return svtkmstd::get<0>(this->Portals).GetNumberOfValues(); }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    ValueType result;
    GetImpl<0, PortalTuple>::Exec(this->Portals, result, index);
    return result;
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    SetImpl<0, PortalTuple>::Exec(this->Portals, value, index);
  }

private:
  PortalTuple Portals;
};

} // namespace internal

template <typename... StorageTags>
struct SVTKM_ALWAYS_EXPORT StorageTagCompositeVec
{
};

namespace internal
{

template <typename ArrayTuple>
struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(1.6, "Use StorageTagCompositeVec instead.")
  StorageTagCompositeVector
{
};

template <typename... ArrayTs>
struct CompositeVectorTraits
{
  // Need to check this here, since this traits struct is used in the
  // ArrayHandleCompositeVector superclass definition before any other
  // static_asserts could be used.
  SVTKM_STATIC_ASSERT_MSG(compvec::AllAreArrayHandles<ArrayTs...>::Value,
                         "Template parameters for ArrayHandleCompositeVector "
                         "must be a list of ArrayHandle types.");

  using ValueType = typename compvec::GetValueType<svtkmstd::tuple<ArrayTs...>>::ValueType;
  using StorageTag = svtkm::cont::StorageTagCompositeVec<typename ArrayTs::StorageTag...>;
  using StorageType = Storage<ValueType, StorageTag>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
};

SVTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename... Arrays>
class Storage<typename compvec::GetValueType<svtkmstd::tuple<Arrays...>>::ValueType,
              StorageTagCompositeVector<svtkmstd::tuple<Arrays...>>>
  : CompositeVectorTraits<Arrays...>::StorageType
{
  using Superclass = typename CompositeVectorTraits<Arrays...>::StorageType;
  using Superclass::Superclass;
};
SVTKM_DEPRECATED_SUPPRESS_END

template <typename T, typename... StorageTags>
class Storage<svtkm::Vec<T, static_cast<svtkm::IdComponent>(sizeof...(StorageTags))>,
              svtkm::cont::StorageTagCompositeVec<StorageTags...>>
{
  using ArrayTuple = svtkmstd::tuple<svtkm::cont::ArrayHandle<T, StorageTags>...>;
  using ForEachArray =
    compvec::ArrayTupleForEach<0, svtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;
  using PortalTypes = compvec::PortalTupleTraits<ArrayTuple>;
  using PortalTupleType = typename PortalTypes::PortalTuple;
  using PortalConstTupleType = typename PortalTypes::PortalConstTuple;

public:
  using ValueType = typename compvec::GetValueType<ArrayTuple>::ValueType;
  using PortalType = ArrayPortalCompositeVector<PortalTupleType>;
  using PortalConstType = ArrayPortalCompositeVector<PortalConstTupleType>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayTuple& arrays)
    : Arrays(arrays)
    , Valid(true)
  {
    using SizeValidator = compvec::ArraySizeValidator<ArrayTuple>;
    if (!SizeValidator::Exec(this->Arrays, this->GetNumberOfValues()))
    {
      throw ErrorBadValue("All arrays must have the same number of values.");
    }
  }

  template <typename... ArrayTypes>
  SVTKM_CONT Storage(const ArrayTypes&... arrays)
    : Arrays(arrays...)
    , Valid(true)
  {
    using SizeValidator = compvec::ArraySizeValidator<ArrayTuple>;
    if (!SizeValidator::Exec(this->Arrays, this->GetNumberOfValues()))
    {
      throw ErrorBadValue("All arrays must have the same number of values.");
    }
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(PortalTypes::GetPortalTupleControl(this->Arrays));
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(PortalTypes::GetPortalConstTupleControl(this->Arrays));
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return svtkmstd::get<0>(this->Arrays).GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Valid);
    ForEachArray::Allocate(this->Arrays, numValues);
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Valid);
    ForEachArray::Shrink(this->Arrays, numValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Valid);
    ForEachArray::ReleaseResources(this->Arrays);
  }

  SVTKM_CONT
  const ArrayTuple& GetArrayTuple() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Arrays;
  }

  SVTKM_CONT
  ArrayTuple& GetArrayTuple()
  {
    SVTKM_ASSERT(this->Valid);
    return this->Arrays;
  }

private:
  ArrayTuple Arrays;
  bool Valid;
};

SVTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename... Arrays, typename DeviceTag>
struct ArrayTransfer<typename compvec::GetValueType<svtkmstd::tuple<Arrays...>>::ValueType,
                     StorageTagCompositeVector<svtkmstd::tuple<Arrays...>>,
                     DeviceTag>
  : ArrayTransfer<typename compvec::GetValueType<svtkmstd::tuple<Arrays...>>::ValueType,
                  typename CompositeVectorTraits<Arrays...>::StorageType,
                  DeviceTag>
{
  using Superclass =
    ArrayTransfer<typename compvec::GetValueType<svtkmstd::tuple<Arrays...>>::ValueType,
                  typename CompositeVectorTraits<Arrays...>::StorageType,
                  DeviceTag>;
  using Superclass::Superclass;
};
SVTKM_DEPRECATED_SUPPRESS_END

template <typename T, typename... StorageTags, typename DeviceTag>
class ArrayTransfer<svtkm::Vec<T, static_cast<svtkm::IdComponent>(sizeof...(StorageTags))>,
                    svtkm::cont::StorageTagCompositeVec<StorageTags...>,
                    DeviceTag>
{
  SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

  using ArrayTuple = svtkmstd::tuple<svtkm::cont::ArrayHandle<T, StorageTags>...>;

public:
  using ValueType = typename compvec::GetValueType<ArrayTuple>::ValueType;

private:
  using ForEachArray =
    compvec::ArrayTupleForEach<0, svtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;
  using StorageTag = svtkm::cont::StorageTagCompositeVec<StorageTags...>;
  using StorageType = internal::Storage<ValueType, StorageTag>;
  using ControlTraits = compvec::PortalTupleTraits<ArrayTuple>;
  using ExecutionTraits = typename ControlTraits::template ExecutionTypes<DeviceTag>;

public:
  using PortalControl = ArrayPortalCompositeVector<typename ControlTraits::PortalTuple>;
  using PortalConstControl = ArrayPortalCompositeVector<typename ControlTraits::PortalConstTuple>;

  using PortalExecution = ArrayPortalCompositeVector<typename ExecutionTraits::PortalTuple>;
  using PortalConstExecution =
    ArrayPortalCompositeVector<typename ExecutionTraits::PortalConstTuple>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData)) const
  {
    return PortalConstExecution(
      ControlTraits::template PrepareForInput<DeviceTag>(this->GetArrayTuple()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(
      ControlTraits::template PrepareForInPlace<DeviceTag>(this->GetArrayTuple()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numValues)
  {
    return PortalExecution(
      ControlTraits::template PrepareForOutput<DeviceTag>(this->GetArrayTuple(), numValues));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues) { ForEachArray::Shrink(this->GetArrayTuple(), numValues); }

  SVTKM_CONT
  void ReleaseResources() { ForEachArray::ReleaseResources(this->GetArrayTuple()); }

  SVTKM_CONT
  const ArrayTuple& GetArrayTuple() const { return this->Storage->GetArrayTuple(); }
  ArrayTuple& GetArrayTuple() { return this->Storage->GetArrayTuple(); }

private:
  StorageType* Storage;
};

} // namespace internal

/// \brief An \c ArrayHandle that combines components from other arrays.
///
/// \c ArrayHandleCompositeVector is a specialization of \c ArrayHandle that
/// derives its content from other arrays. It takes any number of
/// single-component \c ArrayHandle objects and mimics an array that contains
/// vectors with components that come from these delegate arrays.
///
/// The easiest way to create and type an \c ArrayHandleCompositeVector is
/// to use the \c make_ArrayHandleCompositeVector functions.
///
/// The \c ArrayHandleExtractComponent class may be helpful when a desired
/// component is part of an \c ArrayHandle with a \c svtkm::Vec \c ValueType.
///
template <typename... ArrayTs>
class ArrayHandleCompositeVector
  : public ArrayHandle<typename internal::CompositeVectorTraits<ArrayTs...>::ValueType,
                       typename internal::CompositeVectorTraits<ArrayTs...>::StorageTag>
{
private:
  using Traits = internal::CompositeVectorTraits<ArrayTs...>;
  using TupleType = svtkmstd::tuple<ArrayTs...>;
  using StorageType = typename Traits::StorageType;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCompositeVector,
                             (ArrayHandleCompositeVector<ArrayTs...>),
                             (typename Traits::Superclass));

  SVTKM_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType(arrays...))
  {
  }
};

/// Create a composite vector array from other arrays.
///
template <typename... ArrayTs>
SVTKM_CONT ArrayHandleCompositeVector<ArrayTs...> make_ArrayHandleCompositeVector(
  const ArrayTs&... arrays)
{
  SVTKM_STATIC_ASSERT_MSG(internal::compvec::AllAreArrayHandles<ArrayTs...>::Value,
                         "Arguments to make_ArrayHandleCompositeVector must be "
                         "of ArrayHandle types.");
  return ArrayHandleCompositeVector<ArrayTs...>(arrays...);
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

template <typename... AHs>
struct SerializableTypeString<svtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_CompositeVector<" + internal::GetVariadicSerializableTypeString(AHs{}...) + ">";
    return name;
  }
};

template <typename T, typename... STs>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, static_cast<svtkm::IdComponent>(sizeof...(STs))>,
                          svtkm::cont::StorageTagCompositeVec<STs...>>>
  : SerializableTypeString<
      svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<T, STs>...>>
{
};

SVTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename... AHs>
struct SerializableTypeString<svtkm::cont::ArrayHandle<
  typename svtkm::cont::internal::compvec::GetValueType<svtkmstd::tuple<AHs...>>::ValueType,
  svtkm::cont::internal::StorageTagCompositeVector<svtkmstd::tuple<AHs...>>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
};
SVTKM_DEPRECATED_SUPPRESS_END
}
} // svtkm::cont

namespace mangled_diy_namespace
{

namespace internal
{

template <typename Functor, typename TupleType, typename... Args>
inline void TupleForEachImpl(TupleType&& t,
                             std::integral_constant<size_t, 0>,
                             Functor&& f,
                             Args&&... args)
{
  f(svtkmstd::get<0>(t), std::forward<Args>(args)...);
}

template <typename Functor, typename TupleType, size_t Index, typename... Args>
inline void TupleForEachImpl(TupleType&& t,
                             std::integral_constant<size_t, Index>,
                             Functor&& f,
                             Args&&... args)
{
  TupleForEachImpl(std::forward<TupleType>(t),
                   std::integral_constant<size_t, Index - 1>{},
                   std::forward<Functor>(f),
                   std::forward<Args>(args)...);
  f(svtkmstd::get<Index>(t), std::forward<Args>(args)...);
}

template <typename Functor, typename TupleType, typename... Args>
inline void TupleForEach(TupleType&& t, Functor&& f, Args&&... args)
{
  constexpr auto size = svtkmstd::tuple_size<typename std::decay<TupleType>::type>::value;

  TupleForEachImpl(std::forward<TupleType>(t),
                   std::integral_constant<size_t, size - 1>{},
                   std::forward<Functor>(f),
                   std::forward<Args>(args)...);
}
} // internal

template <typename... AHs>
struct Serialization<svtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
private:
  using Type = typename svtkm::cont::ArrayHandleCompositeVector<AHs...>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  struct SaveFunctor
  {
    template <typename AH>
    void operator()(const AH& ah, BinaryBuffer& bb) const
    {
      svtkmdiy::save(bb, ah);
    }
  };

  struct LoadFunctor
  {
    template <typename AH>
    void operator()(AH& ah, BinaryBuffer& bb) const
    {
      svtkmdiy::load(bb, ah);
    }
  };

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    internal::TupleForEach(obj.GetStorage().GetArrayTuple(), SaveFunctor{}, bb);
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkmstd::tuple<AHs...> arrayTuple;
    internal::TupleForEach(arrayTuple, LoadFunctor{}, bb);
    obj = BaseType(typename BaseType::StorageType(arrayTuple));
  }
};

template <typename T, typename... STs>
struct Serialization<
  svtkm::cont::ArrayHandle<svtkm::Vec<T, static_cast<svtkm::IdComponent>(sizeof...(STs))>,
                          svtkm::cont::StorageTagCompositeVec<STs...>>>
  : Serialization<svtkm::cont::ArrayHandleCompositeVector<svtkm::cont::ArrayHandle<T, STs>...>>
{
};

SVTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename... AHs>
struct Serialization<svtkm::cont::ArrayHandle<
  typename svtkm::cont::internal::compvec::GetValueType<svtkmstd::tuple<AHs...>>::ValueType,
  svtkm::cont::internal::StorageTagCompositeVector<svtkmstd::tuple<AHs...>>>>
  : Serialization<svtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
};
SVTKM_DEPRECATED_SUPPRESS_END
} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_ArrayHandleCompositeVector_h
