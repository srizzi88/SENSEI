//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CoordianteSystemTransform_h
#define svtk_m_filter_CoordianteSystemTransform_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/CoordinateSystemTransform.h>

namespace svtkm
{
namespace filter
{
/// \brief
///
/// Generate a coordinate transformation on coordiantes from a dataset.
class CylindricalCoordinateTransform
  : public svtkm::filter::FilterField<CylindricalCoordinateTransform>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  CylindricalCoordinateTransform();

  SVTKM_CONT void SetCartesianToCylindrical() { Worklet.SetCartesianToCylindrical(); }
  SVTKM_CONT void SetCylindricalToCartesian() { Worklet.SetCylindricalToCartesian(); }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  svtkm::worklet::CylindricalCoordinateTransform Worklet;
};

class SphericalCoordinateTransform : public svtkm::filter::FilterField<SphericalCoordinateTransform>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  SphericalCoordinateTransform();

  SVTKM_CONT void SetCartesianToSpherical() { Worklet.SetCartesianToSpherical(); }
  SVTKM_CONT void SetSphericalToCartesian() { Worklet.SetSphericalToCartesian(); }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<T, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    const svtkm::filter::PolicyBase<DerivedPolicy>& policy) const;

private:
  svtkm::worklet::SphericalCoordinateTransform Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/CoordinateSystemTransform.hxx>

#endif // svtk_m_filter_CoordianteSystemTransform_h
