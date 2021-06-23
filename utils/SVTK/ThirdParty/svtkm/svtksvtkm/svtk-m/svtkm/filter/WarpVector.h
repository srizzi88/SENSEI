//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_WarpVector_h
#define svtk_m_filter_WarpVector_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/WarpVector.h>

namespace svtkm
{
namespace filter
{
/// \brief Modify points by moving points along a vector multiplied by
/// the scale factor
///
/// A filter that modifies point coordinates by moving points along a vector
/// multiplied by a scale factor. It's a SVTK-m version of the svtkWarpVector in SVTK.
/// Useful for showing flow profiles or mechanical deformation.
/// This worklet does not modify the input points but generate new point
/// coordinate instance that has been warped.
class WarpVector : public svtkm::filter::FilterField<WarpVector>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;
  using AdditionalFieldStorage =
    svtkm::List<svtkm::cont::ArrayHandleConstant<svtkm::Vec3f_32>::StorageTag,
               svtkm::cont::ArrayHandleConstant<svtkm::Vec3f_64>::StorageTag>;

  SVTKM_CONT
  WarpVector(svtkm::FloatDefault scale);

  //@{
  /// Choose the vector field to operate on. In the warp op A + B *scale, B is
  /// the vector field
  SVTKM_CONT
  void SetVectorField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->VectorFieldName = name;
    this->VectorFieldAssociation = association;
  }

  SVTKM_CONT const std::string& GetVectorFieldName() const { return this->VectorFieldName; }

  SVTKM_CONT svtkm::cont::Field::Association GetVectorFieldAssociation() const
  {
    return this->VectorFieldAssociation;
  }
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::WarpVector Worklet;
  std::string VectorFieldName;
  svtkm::cont::Field::Association VectorFieldAssociation;
  svtkm::FloatDefault Scale;
};
}
}

#include <svtkm/filter/WarpVector.hxx>

#endif // svtk_m_filter_WarpVector_h
