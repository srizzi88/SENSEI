//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PointElevation_hxx
#define svtk_m_filter_PointElevation_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT PointElevation::PointElevation()
  : Worklet()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointElevation::SetLowPoint(svtkm::Float64 x, svtkm::Float64 y, svtkm::Float64 z)
{
  this->Worklet.SetLowPoint(svtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointElevation::SetHighPoint(svtkm::Float64 x,
                                                   svtkm::Float64 y,
                                                   svtkm::Float64 z)
{
  this->Worklet.SetHighPoint(svtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointElevation::SetRange(svtkm::Float64 low, svtkm::Float64 high)
{
  this->Worklet.SetRange(low, high);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet PointElevation::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<svtkm::Float64> outArray;

  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  this->Invoke(this->Worklet, field, outArray);

  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter
#endif
