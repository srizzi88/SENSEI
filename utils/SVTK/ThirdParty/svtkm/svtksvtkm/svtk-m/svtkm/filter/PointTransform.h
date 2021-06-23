//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PointTransform_h
#define svtk_m_filter_PointTransform_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/PointTransform.h>

namespace svtkm
{
namespace filter
{
/// \brief
///
/// Generate scalar field from a dataset.
class PointTransform : public svtkm::filter::FilterField<PointTransform>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  PointTransform();

  void SetTranslation(const svtkm::FloatDefault& tx,
                      const svtkm::FloatDefault& ty,
                      const svtkm::FloatDefault& tz);

  void SetTranslation(const svtkm::Vec3f& v);

  void SetRotation(const svtkm::FloatDefault& angleDegrees, const svtkm::Vec3f& axis);

  void SetRotation(const svtkm::FloatDefault& angleDegrees,
                   const svtkm::FloatDefault& rx,
                   const svtkm::FloatDefault& ry,
                   const svtkm::FloatDefault& rz);

  void SetRotationX(const svtkm::FloatDefault& angleDegrees);

  void SetRotationY(const svtkm::FloatDefault& angleDegrees);

  void SetRotationZ(const svtkm::FloatDefault& angleDegrees);

  void SetScale(const svtkm::FloatDefault& s);

  void SetScale(const svtkm::FloatDefault& sx,
                const svtkm::FloatDefault& sy,
                const svtkm::FloatDefault& sz);

  void SetScale(const svtkm::Vec3f& v);

  void SetTransform(const svtkm::Matrix<svtkm::FloatDefault, 4, 4>& mtx);

  void SetChangeCoordinateSystem(bool flag);
  bool GetChangeCoordinateSystem() const;



  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::PointTransform<svtkm::FloatDefault> Worklet;
  bool ChangeCoordinateSystem;
};
}
} // namespace svtkm::filter

#ifndef svtk_m_filter_PointTransform_hxx
#include <svtkm/filter/PointTransform.hxx>
#endif

#endif // svtk_m_filter_PointTransform_h
