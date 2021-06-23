//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleVirtualCoordinates_h
#define svtk_m_cont_ArrayHandleVirtualCoordinates_h

#include <svtkm/cont/ArrayHandleVirtual.h>

#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/Logging.h>

#include <memory>
#include <type_traits>

namespace svtkm
{
namespace cont
{

/// ArrayHandleVirtualCoordinates is a specialization of ArrayHandle.
class SVTKM_ALWAYS_EXPORT ArrayHandleVirtualCoordinates final
  : public svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleVirtualCoordinates,
                                (svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>));

  template <typename T, typename S>
  explicit ArrayHandleVirtualCoordinates(const svtkm::cont::ArrayHandle<T, S>& ah)
    : svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>(svtkm::cont::make_ArrayHandleCast<ValueType>(ah))
  {
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const svtkm::cont::ArrayHandleVirtualCoordinates& coords,
                 Functor&& f,
                 Args&&... args)
{
  using HandleType = ArrayHandleUniformPointCoordinates;
  if (coords.IsType<HandleType>())
  {
    HandleType uniform = coords.Cast<HandleType>();
    f(uniform, std::forward<Args>(args)...);
  }
  else
  {
    f(coords, std::forward<Args>(args)...);
  }
}


template <>
struct SerializableTypeString<svtkm::cont::ArrayHandleVirtualCoordinates>
{
  static SVTKM_CONT const std::string Get() { return "AH_VirtualCoordinates"; }
};

} // namespace cont
} // namespace svtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct Serialization<svtkm::cont::ArrayHandleVirtualCoordinates>
{
private:
  using Type = svtkm::cont::ArrayHandleVirtualCoordinates;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  using BasicCoordsType = svtkm::cont::ArrayHandle<svtkm::Vec3f>;
  using RectilinearCoordsArrayType =
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& baseObj)
  {
    Type obj(baseObj);
    const svtkm::cont::internal::detail::StorageVirtual* storage =
      obj.GetStorage().GetStorageVirtual();
    if (obj.IsType<svtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      using HandleType = svtkm::cont::ArrayHandleUniformPointCoordinates;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<HandleType>::Get());
      svtkmdiy::save(bb, array->GetHandle());
    }
    else if (obj.IsType<RectilinearCoordsArrayType>())
    {
      using HandleType = RectilinearCoordsArrayType;
      using T = typename HandleType::ValueType;
      using S = typename HandleType::StorageTag;
      auto array = storage->Cast<svtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<HandleType>::Get());
      svtkmdiy::save(bb, array->GetHandle());
    }
    else
    {
      svtkmdiy::save(bb, svtkm::cont::SerializableTypeString<BasicCoordsType>::Get());
      svtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
    }
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::string typeString;
    svtkmdiy::load(bb, typeString);

    if (typeString ==
        svtkm::cont::SerializableTypeString<svtkm::cont::ArrayHandleUniformPointCoordinates>::Get())
    {
      svtkm::cont::ArrayHandleUniformPointCoordinates array;
      svtkmdiy::load(bb, array);
      obj = svtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == svtkm::cont::SerializableTypeString<RectilinearCoordsArrayType>::Get())
    {
      RectilinearCoordsArrayType array;
      svtkmdiy::load(bb, array);
      obj = svtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else if (typeString == svtkm::cont::SerializableTypeString<BasicCoordsType>::Get())
    {
      BasicCoordsType array;
      svtkmdiy::load(bb, array);
      obj = svtkm::cont::ArrayHandleVirtualCoordinates(array);
    }
    else
    {
      throw svtkm::cont::ErrorBadType(
        "Error deserializing ArrayHandleVirtualCoordinates. TypeString: " + typeString);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif // svtk_m_cont_ArrayHandleVirtualCoordinates_h
