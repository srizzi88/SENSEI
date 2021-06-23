//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleView_h
#define svtk_m_cont_ArrayHandleView_h

#include <svtkm/Assert.h>
#include <svtkm/Deprecated.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayPortal.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

template <typename TargetPortalType>
class ArrayPortalView
{
  using Writable = svtkm::internal::PortalSupportsSets<TargetPortalType>;

public:
  using ValueType = typename TargetPortalType::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalView() {}

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalView(const TargetPortalType& targetPortal, svtkm::Id startIndex, svtkm::Id numValues)
    : TargetPortal(targetPortal)
    , StartIndex(startIndex)
    , NumValues(numValues)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherPortalType>
  SVTKM_EXEC_CONT ArrayPortalView(const ArrayPortalView<OtherPortalType>& otherPortal)
    : TargetPortal(otherPortal.GetTargetPortal())
    , StartIndex(otherPortal.GetStartIndex())
    , NumValues(otherPortal.GetNumberOfValues())
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumValues; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return this->TargetPortal.Get(index + this->StartIndex); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    this->TargetPortal.Set(index + this->StartIndex, value);
  }

  SVTKM_EXEC_CONT
  const TargetPortalType& GetTargetPortal() const { return this->TargetPortal; }
  SVTKM_EXEC_CONT
  svtkm::Id GetStartIndex() const { return this->StartIndex; }

private:
  TargetPortalType TargetPortal;
  svtkm::Id StartIndex;
  svtkm::Id NumValues;
};

} // namespace internal

template <typename StorageTag>
struct SVTKM_ALWAYS_EXPORT StorageTagView
{
};

namespace internal
{

namespace detail
{

template <typename T, typename ArrayOrStorage, bool IsArrayType>
struct ViewTypeArgImpl;

template <typename T, typename Storage>
struct ViewTypeArgImpl<T, Storage, false>
{
  using StorageTag = Storage;
  using ArrayHandle = svtkm::cont::ArrayHandle<T, StorageTag>;
};

template <typename T, typename Array>
struct ViewTypeArgImpl<T, Array, true>
{
  SVTKM_STATIC_ASSERT_MSG((std::is_same<T, typename Array::ValueType>::value),
                         "Used array with wrong type in ArrayHandleView.");
  using StorageTag SVTKM_DEPRECATED(1.6,
                                   "Use storage tag instead of array handle in StorageTagView.") =
    typename Array::StorageTag;
  using ArrayHandle SVTKM_DEPRECATED(1.6,
                                    "Use storage tag instead of array handle in StorageTagView.") =
    svtkm::cont::ArrayHandle<T, typename Array::StorageTag>;
};

template <typename T, typename ArrayOrStorage>
struct ViewTypeArg
  : ViewTypeArgImpl<T,
                    ArrayOrStorage,
                    svtkm::cont::internal::ArrayHandleCheck<ArrayOrStorage>::type::value>
{
};

} // detail

template <typename T, typename ST>
class Storage<T, StorageTagView<ST>>
{
  using ArrayHandleType = typename detail::ViewTypeArg<T, ST>::ArrayHandle;

public:
  using ValueType = T;

  using PortalType = ArrayPortalView<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalView<typename ArrayHandleType::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& array, svtkm::Id startIndex, svtkm::Id numValues)
    : Array(array)
    , StartIndex(startIndex)
    , NumValues(numValues)
    , Valid(true)
  {
    SVTKM_ASSERT(this->StartIndex >= 0);
    SVTKM_ASSERT((this->StartIndex + this->NumValues) <= this->Array.GetNumberOfValues());
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->StartIndex, this->NumValues);
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->StartIndex, this->NumValues);
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumValues; }

  SVTKM_CONT
  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorInternal("ArrayHandleView should not be allocated explicitly. ");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(this->Valid);
    if (numberOfValues > this->NumValues)
    {
      throw svtkm::cont::ErrorBadValue("Shrink method cannot be used to grow array.");
    }

    this->NumValues = numberOfValues;
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  // Required for later use in ArrayTransfer class.
  SVTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array;
  }
  SVTKM_CONT
  svtkm::Id GetStartIndex() const { return this->StartIndex; }

private:
  ArrayHandleType Array;
  svtkm::Id StartIndex;
  svtkm::Id NumValues;
  bool Valid;
};

template <typename T, typename ST, typename Device>
class ArrayTransfer<T, StorageTagView<ST>, Device>
{
private:
  using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagView<ST>>;
  using ArrayHandleType = typename detail::ViewTypeArg<T, ST>::ArrayHandle;

public:
  using ValueType = T;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalView<typename ArrayHandleType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalView<typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , StartIndex(storage->GetStartIndex())
    , NumValues(storage->GetNumberOfValues())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumValues; }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(
      this->Array.PrepareForInput(Device()), this->StartIndex, this->NumValues);
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(
      this->Array.PrepareForInPlace(Device()), this->StartIndex, this->NumValues);
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    if (numberOfValues != this->GetNumberOfValues())
    {
      throw svtkm::cont::ErrorBadValue(
        "An ArrayHandleView can be used as an output array, "
        "but it cannot be resized. Make sure the index array is sized "
        "to the appropriate length before trying to prepare for output.");
    }

    // We cannot practically allocate ValueArray because we do not know the
    // range of indices. We try to check by seeing if ValueArray has no
    // entries, which clearly indicates that it is not allocated. Otherwise,
    // we have to assume the allocation is correct.
    if ((numberOfValues > 0) && (this->Array.GetNumberOfValues() < 1))
    {
      throw svtkm::cont::ErrorBadValue(
        "The value array must be pre-allocated before it is used for the "
        "output of ArrayHandlePermutation.");
    }

    return PortalExecution(this->Array.PrepareForOutput(this->Array.GetNumberOfValues(), Device()),
                           this->StartIndex,
                           this->NumValues);
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // No implementation necessary
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->NumValues = numberOfValues; }

  SVTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  svtkm::Id StartIndex;
  svtkm::Id NumValues;
};

} // namespace internal

template <typename ArrayHandleType>
class ArrayHandleView
  : public svtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   StorageTagView<typename ArrayHandleType::StorageTag>>
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleView,
    (ArrayHandleView<ArrayHandleType>),
    (svtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                             StorageTagView<typename ArrayHandleType::StorageTag>>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleView(const ArrayHandleType& array, svtkm::Id startIndex, svtkm::Id numValues)
    : Superclass(StorageType(array, startIndex, numValues))
  {
  }
};

template <typename ArrayHandleType>
ArrayHandleView<ArrayHandleType> make_ArrayHandleView(const ArrayHandleType& array,
                                                      svtkm::Id startIndex,
                                                      svtkm::Id numValues)
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  return ArrayHandleView<ArrayHandleType>(array, startIndex, numValues);
}
}
} // namespace svtkm::cont

#endif //svtk_m_cont_ArrayHandleView_h
