//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_WarpScalar_h
#define svtk_m_filter_WarpScalar_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/WarpScalar.h>

namespace svtkm
{
namespace filter
{
/// \brief Modify points by moving points along point normals by the scalar
/// amount times the scalar factor.
///
/// A filter that modifies point coordinates by moving points along point normals
/// by the scalar amount times the scalar factor.
/// It's a SVTK-m version of the svtkWarpScalar in SVTK.
/// Useful for creating carpet or x-y-z plots.
/// It doesn't modify the point coordinates, but creates a new point coordinates that have been warped.
class WarpScalar : public svtkm::filter::FilterField<WarpScalar>
{
public:
  // WarpScalar can only applies to Float and Double Vec3 arrays
  using SupportedTypes = svtkm::TypeListFieldVec3;

  // WarpScalar often operates on a constant normal value
  using AdditionalFieldStorage =
    svtkm::List<svtkm::cont::ArrayHandleConstant<svtkm::Vec3f_32>::StorageTag,
               svtkm::cont::ArrayHandleConstant<svtkm::Vec3f_64>::StorageTag>;

  SVTKM_CONT
  WarpScalar(svtkm::FloatDefault scaleAmount);

  //@{
  /// Choose the secondary field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, B is the secondary field
  SVTKM_CONT
  void SetNormalField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->NormalFieldName = name;
    this->NormalFieldAssociation = association;
  }

  SVTKM_CONT const std::string& GetNormalFieldName() const { return this->NormalFieldName; }

  SVTKM_CONT svtkm::cont::Field::Association GetNormalFieldAssociation() const
  {
    return this->NormalFieldAssociation;
  }
  //@}

  //@{
  /// Choose the scalar factor field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, scalarFactor is the scalar factor field.
  SVTKM_CONT
  void SetScalarFactorField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->ScalarFactorFieldName = name;
    this->ScalarFactorFieldAssociation = association;
  }

  SVTKM_CONT const std::string& GetScalarFactorFieldName() const
  {
    return this->ScalarFactorFieldName;
  }

  SVTKM_CONT svtkm::cont::Field::Association GetScalarFactorFieldAssociation() const
  {
    return this->ScalarFactorFieldAssociation;
  }
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::WarpScalar Worklet;
  std::string NormalFieldName;
  svtkm::cont::Field::Association NormalFieldAssociation;
  std::string ScalarFactorFieldName;
  svtkm::cont::Field::Association ScalarFactorFieldAssociation;
  svtkm::FloatDefault ScaleAmount;
};
}
}

#include <svtkm/filter/WarpScalar.hxx>

#endif // svtk_m_filter_WarpScalar_h
