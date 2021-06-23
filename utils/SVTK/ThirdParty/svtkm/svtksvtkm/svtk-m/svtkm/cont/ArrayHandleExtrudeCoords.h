//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleExtrudeCoords_h
#define svtk_m_cont_ArrayHandleExtrudeCoords_h

#include <svtkm/cont/StorageExtrude.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/CoordinateSystem.hxx>

namespace svtkm
{
namespace cont
{

template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayHandleExtrudeCoords
  : public svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, internal::StorageTagExtrude>
{

  using StorageType = svtkm::cont::internal::Storage<svtkm::Vec<T, 3>, internal::StorageTagExtrude>;

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtrudeCoords,
    (ArrayHandleExtrudeCoords<T>),
    (svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, internal::StorageTagExtrude>));

  ArrayHandleExtrudeCoords(const StorageType& storage)
    : Superclass(storage)
  {
  }

  svtkm::Id GetNumberOfPointsPerPlane() const { return (this->GetStorage().GetLength() / 2); }
  svtkm::Int32 GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }
  bool GetUseCylindrical() const { return this->GetStorage().GetUseCylindrical(); }
  const svtkm::cont::ArrayHandle<T>& GetArray() const { return this->GetStorage().Array; }
};

template <typename T>
svtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const svtkm::cont::ArrayHandle<T> arrHandle,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical)
{
  using StorageType = svtkm::cont::internal::Storage<svtkm::Vec<T, 3>, internal::StorageTagExtrude>;
  auto storage = StorageType(arrHandle, numberOfPlanes, cylindrical);
  return ArrayHandleExtrudeCoords<T>(storage);
}

template <typename T>
svtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const T* array,
  svtkm::Id length,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  using StorageType = svtkm::cont::internal::Storage<svtkm::Vec<T, 3>, internal::StorageTagExtrude>;
  if (copy == svtkm::CopyFlag::Off)
  {
    return ArrayHandleExtrudeCoords<T>(StorageType(array, length, numberOfPlanes, cylindrical));
  }
  else
  {
    auto storage = StorageType(
      svtkm::cont::make_ArrayHandle(array, length, svtkm::CopyFlag::On), numberOfPlanes, cylindrical);
    return ArrayHandleExtrudeCoords<T>(storage);
  }
}

template <typename T>
svtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const std::vector<T>& array,
  svtkm::Int32 numberOfPlanes,
  bool cylindrical,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandleExtrudeCoords(
      &array.front(), static_cast<svtkm::Id>(array.size()), numberOfPlanes, cylindrical, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleExtrudeCoords<T>();
  }
}
}
} // end namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<svtkm::cont::ArrayHandleExtrudeCoords<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtrudeCoords<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandleExtrudeCoords<T>>
{
private:
  using Type = svtkm::cont::ArrayHandleExtrudeCoords<T>;

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

    ah = svtkm::cont::make_ArrayHandleExtrudeCoords(array, numberOfPlanes, isCylindrical);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif
