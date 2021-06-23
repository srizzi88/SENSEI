//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CoordianteSystemTransform_hxx
#define svtk_m_filter_CoordianteSystemTransform_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT CylindricalCoordinateTransform::CylindricalCoordinateTransform()
  : Worklet()
{
  this->SetOutputFieldName("cylindricalCoordinateSystemTransform");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet CylindricalCoordinateTransform::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  svtkm::cont::ArrayHandle<T> outArray;
  this->Worklet.Run(field, outArray);
  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT SphericalCoordinateTransform::SphericalCoordinateTransform()
  : Worklet()
{
  this->SetOutputFieldName("sphericalCoordinateSystemTransform");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet SphericalCoordinateTransform::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>&) const
{
  svtkm::cont::ArrayHandle<T> outArray;
  Worklet.Run(field, outArray);
  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter

#endif
