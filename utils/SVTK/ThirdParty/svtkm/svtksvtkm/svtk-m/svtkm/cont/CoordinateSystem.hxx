//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CoordinateSystem_hxx
#define svtk_m_cont_CoordinateSystem_hxx

#include <svtkm/cont/CoordinateSystem.h>

namespace svtkm
{
namespace cont
{
namespace detail
{

struct MakeArrayHandleVirtualCoordinatesFunctor
{
  template <typename StorageTag>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<svtkm::Vec3f_32, StorageTag>& array,
                            ArrayHandleVirtualCoordinates& output) const
  {
    output = svtkm::cont::ArrayHandleVirtualCoordinates(array);
  }

  template <typename StorageTag>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<svtkm::Vec3f_64, StorageTag>& array,
                            ArrayHandleVirtualCoordinates& output) const
  {
    output = svtkm::cont::ArrayHandleVirtualCoordinates(array);
  }
};

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandleVirtualCoordinates MakeArrayHandleVirtualCoordinates(
  const svtkm::cont::VariantArrayHandleBase<TypeList>& array)
{
  svtkm::cont::ArrayHandleVirtualCoordinates output;
  svtkm::cont::CastAndCall(array.ResetTypes(svtkm::TypeListFieldVec3{}),
                          MakeArrayHandleVirtualCoordinatesFunctor{},
                          output);
  return output;
}
} // namespace detail

template <typename TypeList>
SVTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::VariantArrayHandleBase<TypeList>& data)
  : Superclass(name, Association::POINTS, detail::MakeArrayHandleVirtualCoordinates(data))
{
}

template <typename T, typename Storage>
SVTKM_CONT CoordinateSystem::CoordinateSystem(std::string name,
                                             const svtkm::cont::ArrayHandle<T, Storage>& data)
  : Superclass(name, Association::POINTS, svtkm::cont::ArrayHandleVirtualCoordinates(data))
{
}

template <typename T, typename Storage>
SVTKM_CONT void CoordinateSystem::SetData(const svtkm::cont::ArrayHandle<T, Storage>& newdata)
{
  this->SetData(svtkm::cont::ArrayHandleVirtualCoordinates(newdata));
}

template <typename TypeList>
SVTKM_CONT void CoordinateSystem::SetData(
  const svtkm::cont::VariantArrayHandleBase<TypeList>& newdata)
{
  this->SetData(detail::MakeArrayHandleVirtualCoordinates(newdata));
}
}
}
#endif
