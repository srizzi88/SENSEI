//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleVirtual_hxx
#define svtk_m_cont_ArrayHandleVirtual_hxx

#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/TryExecute.h>

namespace svtkm
{
namespace cont
{

template <typename T>
template <typename ArrayHandleType>
ArrayHandleType inline ArrayHandleVirtual<T>::CastToType(
  std::true_type svtkmNotUsed(valueTypesMatch),
  std::false_type svtkmNotUsed(notFromArrayHandleVirtual)) const
{
  auto* storage = this->GetStorage().GetStorageVirtual();
  if (!storage)
  {
    SVTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", svtkm::cont::TypeToString<ArrayHandleType>());
  }
  using S = typename ArrayHandleType::StorageTag;
  const auto* castStorage =
    storage->template Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
  return castStorage->GetHandle();
}
}
} // namespace svtkm::cont


#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandleVirtual<T>>
{

  static SVTKM_CONT void save(svtkmdiy::BinaryBuffer& bb,
                             const svtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    svtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, svtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    svtkm::cont::ArrayHandle<T> array;
    svtkmdiy::load(bb, array);
    obj = std::move(svtkm::cont::ArrayHandleVirtual<T>{ array });
  }
};

template <typename T>
struct IntAnySerializer
{
  using CountingType = svtkm::cont::ArrayHandleCounting<T>;
  using ConstantType = svtkm::cont::ArrayHandleConstant<T>;
  using BasicType = svtkm::cont::ArrayHandle<T>;

  static SVTKM_CONT void save(svtkmdiy::BinaryBuffer& bb,
                             const svtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    if (obj.template IsType<CountingType>())
    {
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<CountingType>::Get());

      using S = typename CountingType::StorageTag;
      const svtkm::cont::internal::detail::StorageVirtual* storage =
        obj.GetStorage().GetStorageVirtual();
      auto* castStorage = storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      svtkmdiy::save(bb, castStorage->GetHandle());
    }
    else if (obj.template IsType<ConstantType>())
    {
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<ConstantType>::Get());

      using S = typename ConstantType::StorageTag;
      const svtkm::cont::internal::detail::StorageVirtual* storage =
        obj.GetStorage().GetStorageVirtual();
      auto* castStorage = storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      svtkmdiy::save(bb, castStorage->GetHandle());
    }
    else
    {
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<BasicType>::Get());
      svtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, svtkm::cont::ArrayHandleVirtual<T>& obj)
  {
    std::string typeString;
    svtkmdiy::load(bb, typeString);

    if (typeString == svtkm::cont::SerializableTypeString<CountingType>::Get())
    {
      CountingType array;
      svtkmdiy::load(bb, array);
      obj = std::move(svtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else if (typeString == svtkm::cont::SerializableTypeString<ConstantType>::Get())
    {
      ConstantType array;
      svtkmdiy::load(bb, array);
      obj = std::move(svtkm::cont::ArrayHandleVirtual<T>{ array });
    }
    else
    {
      svtkm::cont::ArrayHandle<T> array;
      svtkmdiy::load(bb, array);
      obj = std::move(svtkm::cont::ArrayHandleVirtual<T>{ array });
    }
  }
};


template <>
struct Serialization<svtkm::cont::ArrayHandleVirtual<svtkm::UInt8>>
  : public IntAnySerializer<svtkm::UInt8>
{
};
template <>
struct Serialization<svtkm::cont::ArrayHandleVirtual<svtkm::Int32>>
  : public IntAnySerializer<svtkm::Int32>
{
};
template <>
struct Serialization<svtkm::cont::ArrayHandleVirtual<svtkm::Int64>>
  : public IntAnySerializer<svtkm::Int64>
{
};

template <typename T>
struct Serialization<svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>>
  : public Serialization<svtkm::cont::ArrayHandleVirtual<T>>
{
};

} // mangled_diy_namespace

#endif
