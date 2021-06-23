//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/CoordinateSystem.hxx>

namespace svtkm
{
namespace cont
{

SVTKM_CONT CoordinateSystem::CoordinateSystem()
  : Superclass()
{
}

SVTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& data)
  : Superclass(name, Association::POINTS, data)
{
}

/// This constructor of coordinate system sets up a regular grid of points.
///
SVTKM_CONT
CoordinateSystem::CoordinateSystem(std::string name,
                                   svtkm::Id3 dimensions,
                                   svtkm::Vec3f origin,
                                   svtkm::Vec3f spacing)
  : Superclass(name,
               Association::POINTS,
               svtkm::cont::ArrayHandleVirtualCoordinates(
                 svtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing)))
{
}

SVTKM_CONT
svtkm::cont::ArrayHandleVirtualCoordinates CoordinateSystem::GetData() const
{
  return this->Superclass::GetData().Cast<svtkm::cont::ArrayHandleVirtualCoordinates>();
}

SVTKM_CONT
void CoordinateSystem::SetData(const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& newdata)
{
  this->Superclass::SetData(newdata);
}

SVTKM_CONT
void CoordinateSystem::PrintSummary(std::ostream& out) const
{
  out << "    Coordinate System ";
  this->Superclass::PrintSummary(out);
}

template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<svtkm::Vec<float, 3>>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<svtkm::Vec<double, 3>>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<
    svtkm::Vec3f,
    svtkm::cont::StorageTagImplicit<svtkm::internal::ArrayPortalUniformPointCoordinates>>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<
    svtkm::Vec3f_32,
    svtkm::cont::StorageTagCartesianProduct<svtkm::cont::StorageTagBasic,
                                           svtkm::cont::StorageTagBasic,
                                           svtkm::cont::StorageTagBasic>>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<
    svtkm::Vec3f_64,
    svtkm::cont::StorageTagCartesianProduct<svtkm::cont::StorageTagBasic,
                                           svtkm::cont::StorageTagBasic,
                                           svtkm::cont::StorageTagBasic>>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<
    svtkm::Vec3f_32,
    typename svtkm::cont::ArrayHandleCompositeVector<
      svtkm::cont::ArrayHandle<svtkm::Float32, svtkm::cont::StorageTagBasic>,
      svtkm::cont::ArrayHandle<svtkm::Float32, svtkm::cont::StorageTagBasic>,
      svtkm::cont::ArrayHandle<svtkm::Float32, svtkm::cont::StorageTagBasic>>::StorageTag>&);
template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const svtkm::cont::ArrayHandle<
    svtkm::Vec3f_64,
    typename svtkm::cont::ArrayHandleCompositeVector<
      svtkm::cont::ArrayHandle<svtkm::Float64, svtkm::cont::StorageTagBasic>,
      svtkm::cont::ArrayHandle<svtkm::Float64, svtkm::cont::StorageTagBasic>,
      svtkm::cont::ArrayHandle<svtkm::Float64, svtkm::cont::StorageTagBasic>>::StorageTag>&);

template SVTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(std::string name,
                                                             const svtkm::cont::VariantArrayHandle&);

template SVTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const svtkm::cont::ArrayHandle<svtkm::Vec<float, 3>>&);
template SVTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const svtkm::cont::ArrayHandle<svtkm::Vec<double, 3>>&);
template SVTKM_CONT_EXPORT void CoordinateSystem::SetData(const svtkm::cont::VariantArrayHandle&);
}
} // namespace svtkm::cont
