//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_ArrayHandleReverse_h
#define svtk_m_cont_ArrayHandleReverse_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>

#include <svtkm/Deprecated.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

template <typename PortalType>
class SVTKM_ALWAYS_EXPORT ArrayPortalReverse
{
  using Writable = svtkm::internal::PortalSupportsSets<PortalType>;

public:
  using ValueType = typename PortalType::ValueType;

  SVTKM_EXEC_CONT
  ArrayPortalReverse()
    : portal()
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalReverse(const PortalType& p)
    : portal(p)
  {
  }

  template <typename OtherPortal>
  SVTKM_EXEC_CONT ArrayPortalReverse(const ArrayPortalReverse<OtherPortal>& src)
    : portal(src.GetPortal())
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->portal.GetNumberOfValues(); }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    return this->portal.Get(portal.GetNumberOfValues() - index - 1);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    this->portal.Set(portal.GetNumberOfValues() - index - 1, value);
  }

private:
  PortalType portal;
};
}

template <typename StorageTag>
class SVTKM_ALWAYS_EXPORT StorageTagReverse
{
};

namespace internal
{

namespace detail
{

template <typename T, typename ArrayOrStorage, bool IsArrayType>
struct ReverseTypeArgImpl;

template <typename T, typename Storage>
struct ReverseTypeArgImpl<T, Storage, false>
{
  using StorageTag = Storage;
  using ArrayHandle = svtkm::cont::ArrayHandle<T, StorageTag>;
};

template <typename T, typename Array>
struct ReverseTypeArgImpl<T, Array, true>
{
  SVTKM_STATIC_ASSERT_MSG((std::is_same<T, typename Array::ValueType>::value),
                         "Used array with wrong type in ArrayHandleReverse.");
  using StorageTag SVTKM_DEPRECATED(
    1.6,
    "Use storage tag instead of array handle in StorageTagReverse.") = typename Array::StorageTag;
  using ArrayHandle SVTKM_DEPRECATED(
    1.6,
    "Use storage tag instead of array handle in StorageTagReverse.") =
    svtkm::cont::ArrayHandle<T, typename Array::StorageTag>;
};

template <typename T, typename ArrayOrStorage>
struct ReverseTypeArg
  : ReverseTypeArgImpl<T,
                       ArrayOrStorage,
                       svtkm::cont::internal::ArrayHandleCheck<ArrayOrStorage>::type::value>
{
};

} // namespace detail

template <typename T, typename ST>
class Storage<T, StorageTagReverse<ST>>
{
public:
  using ValueType = T;
  using ArrayHandleType = typename detail::ReverseTypeArg<T, ST>::ArrayHandle;
  using PortalType = ArrayPortalReverse<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalReverse<typename ArrayHandleType::PortalConstControl>;

  SVTKM_CONT
  Storage()
    : Array()
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& a)
    : Array(a)
  {
  }


  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array.GetPortalConstControl());
  }

  SVTKM_CONT
  PortalType GetPortal() { return PortalType(this->Array.GetPortalControl()); }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues) { return this->Array.Allocate(numberOfValues); }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { return this->Array.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  SVTKM_CONT
  const ArrayHandleType& GetArray() const { return this->Array; }

private:
  ArrayHandleType Array;
}; // class storage

template <typename T, typename ST, typename Device>
class ArrayTransfer<T, StorageTagReverse<ST>, Device>
{
private:
  using StorageTag = StorageTagReverse<ST>;
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag>;
  using ArrayHandleType = typename detail::ReverseTypeArg<T, ST>::ArrayHandle;

public:
  using ValueType = T;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // not need to implement
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
};

} // namespace internal

/// \brief Reverse the order of an array, on demand.
///
/// ArrayHandleReverse is a specialization of ArrayHandle. Given an ArrayHandle,
/// it creates a new handle that returns the elements of the array in reverse
/// order (i.e. from end to beginning).
///
template <typename ArrayHandleType>
class ArrayHandleReverse
  : public svtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   StorageTagReverse<typename ArrayHandleType::StorageTag>>

{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleReverse,
    (ArrayHandleReverse<ArrayHandleType>),
    (svtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                             StorageTagReverse<typename ArrayHandleType::StorageTag>>));

public:
  ArrayHandleReverse(const ArrayHandleType& handle)
    : Superclass(handle)
  {
  }
};

/// make_ArrayHandleReverse is convenience function to generate an
/// ArrayHandleReverse.
///
template <typename HandleType>
SVTKM_CONT ArrayHandleReverse<HandleType> make_ArrayHandleReverse(const HandleType& handle)
{
  return ArrayHandleReverse<HandleType>(handle);
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

template <typename AH>
struct SerializableTypeString<svtkm::cont::ArrayHandleReverse<AH>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Reverse<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST>
struct SerializableTypeString<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagReverse<ST>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<T, ST>>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<svtkm::cont::ArrayHandleReverse<AH>>
{
private:
  using Type = svtkm::cont::ArrayHandleReverse<AH>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    svtkmdiy::save(bb, obj.GetStorage().GetArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    svtkmdiy::load(bb, array);
    obj = svtkm::cont::make_ArrayHandleReverse(array);
  }
};

template <typename T, typename ST>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagReverse<ST>>>
  : Serialization<svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<T, ST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // svtk_m_cont_ArrayHandleReverse_h
