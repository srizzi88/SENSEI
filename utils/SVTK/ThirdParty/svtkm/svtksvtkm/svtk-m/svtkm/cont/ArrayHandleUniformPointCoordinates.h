//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleUniformPointCoordinates_h
#define svtk_m_cont_ArrayHandleUniformPointCoordinates_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageImplicit.h>
#include <svtkm/internal/ArrayPortalUniformPointCoordinates.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagUniformPoints
{
};

namespace internal
{

using StorageTagUniformPointsSuperclass =
  svtkm::cont::StorageTagImplicit<svtkm::internal::ArrayPortalUniformPointCoordinates>;

template <>
struct Storage<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints>
  : Storage<svtkm::Vec3f, StorageTagUniformPointsSuperclass>
{
  using Superclass = Storage<svtkm::Vec3f, StorageTagUniformPointsSuperclass>;

  using Superclass::Superclass;
};

template <typename Device>
struct ArrayTransfer<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints, Device>
  : ArrayTransfer<svtkm::Vec3f, StorageTagUniformPointsSuperclass, Device>
{
  using Superclass = ArrayTransfer<svtkm::Vec3f, StorageTagUniformPointsSuperclass, Device>;

  using Superclass::Superclass;
};

} // namespace internal

/// ArrayHandleUniformPointCoordinates is a specialization of ArrayHandle. It
/// contains the information necessary to compute the point coordinates in a
/// uniform orthogonal grid (extent, origin, and spacing) and implicitly
/// computes these coordinates in its array portal.
///
class SVTKM_ALWAYS_EXPORT ArrayHandleUniformPointCoordinates
  : public svtkm::cont::ArrayHandle<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints>
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS_NT(
    ArrayHandleUniformPointCoordinates,
    (svtkm::cont::ArrayHandle<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleUniformPointCoordinates(svtkm::Id3 dimensions,
                                     ValueType origin = ValueType(0.0f, 0.0f, 0.0f),
                                     ValueType spacing = ValueType(1.0f, 1.0f, 1.0f))
    : Superclass(StorageType(
        svtkm::internal::ArrayPortalUniformPointCoordinates(dimensions, origin, spacing)))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleUniformPointCoordinates() {}
};
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <>
struct SerializableTypeString<svtkm::cont::ArrayHandleUniformPointCoordinates>
{
  static SVTKM_CONT const std::string Get() { return "AH_UniformPointCoordinates"; }
};

template <>
struct SerializableTypeString<
  svtkm::cont::ArrayHandle<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints>>
  : SerializableTypeString<svtkm::cont::ArrayHandleUniformPointCoordinates>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<svtkm::cont::ArrayHandleUniformPointCoordinates>
{
private:
  using Type = svtkm::cont::ArrayHandleUniformPointCoordinates;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.GetPortalConstControl();
    svtkmdiy::save(bb, portal.GetDimensions());
    svtkmdiy::save(bb, portal.GetOrigin());
    svtkmdiy::save(bb, portal.GetSpacing());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    svtkm::Id3 dims;
    typename BaseType::ValueType origin, spacing;

    svtkmdiy::load(bb, dims);
    svtkmdiy::load(bb, origin);
    svtkmdiy::load(bb, spacing);

    obj = svtkm::cont::ArrayHandleUniformPointCoordinates(dims, origin, spacing);
  }
};

template <>
struct Serialization<svtkm::cont::ArrayHandle<svtkm::Vec3f, svtkm::cont::StorageTagUniformPoints>>
  : Serialization<svtkm::cont::ArrayHandleUniformPointCoordinates>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_+m_cont_ArrayHandleUniformPointCoordinates_h
