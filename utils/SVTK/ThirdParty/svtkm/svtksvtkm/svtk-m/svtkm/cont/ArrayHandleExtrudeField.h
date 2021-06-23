//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleExtrudeField_h
#define svtk_m_cont_ArrayHandleExtrudeField_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageExtrude.h>

namespace svtkm
{
namespace cont
{

template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayHandleExtrudeField
  : public svtkm::cont::ArrayHandle<T, internal::StorageTagExtrude>
{
  using StorageType = svtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleExtrudeField,
                             (ArrayHandleExtrudeField<T>),
                             (svtkm::cont::ArrayHandle<T, internal::StorageTagExtrude>));

  ArrayHandleExtrudeField(const StorageType& storage)
    : Superclass(storage)
  {
  }

  svtkm::Int32 GetNumberOfValuesPerPlane() const
  {
    return this->GetStorage().GetNumberOfValuesPerPlane();
  }

  svtkm::Int32 GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }
  bool GetUseCylindrical() const { return this->GetStorage().GetUseCylindrical(); }
  const svtkm::cont::ArrayHandle<T>& GetArray() const { return this->GetStorage().Array; }
};

template <typename T>
svtkm::cont::ArrayHandleExtrudeField<T> make_ArrayHandleExtrudeField(
  const svtkm::cont::ArrayHandle<T>& array,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical)
{
  using StorageType = svtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;
  auto storage = StorageType{ array, numberOfPlanes, cylindrical };
  return ArrayHandleExtrudeField<T>(storage);
}

template <typename T>
svtkm::cont::ArrayHandleExtrudeField<T> make_ArrayHandleExtrudeField(
  const T* array,
  svtkm::Id length,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  using StorageType = svtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;
  auto storage =
    StorageType(svtkm::cont::make_ArrayHandle(array, length, copy), numberOfPlanes, cylindrical);
  return ArrayHandleExtrudeField<T>(storage);
}

template <typename T>
svtkm::cont::ArrayHandleExtrudeField<T> make_ArrayHandleExtrudeField(
  const std::vector<T>& array,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandleExtrudeField(
      array.data(), static_cast<svtkm::Id>(array.size()), numberOfPlanes, cylindrical, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleExtrudeField<T>();
  }
}
}
} // svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandleExtrudeField<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtrudeField<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandleExtrudeField<T>>
{
private:
  using Type = svtkm::cont::ArrayHandleExtrudeField<T>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& ah)
  {
    svtkmdiy::save(bb, ah.GetNumberOfPlanes());
    svtkmdiy::save(bb, ah.GetUseCylindrical());
    svtkmdiy::save(bb, ah.GetArray());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& ah)
  {
    svtkm::Int32 numberOfPlanes;
    bool isCylindrical;
    svtkm::cont::ArrayHandle<T> array;

    svtkmdiy::load(bb, numberOfPlanes);
    svtkmdiy::load(bb, isCylindrical);
    svtkmdiy::load(bb, array);

    ah = svtkm::cont::make_ArrayHandleExtrudeField(array, numberOfPlanes, isCylindrical);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif
