//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_DotProduct_hxx
#define svtk_m_filter_DotProduct_hxx

#include <svtkm/cont/ArrayHandleCast.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT DotProduct::DotProduct()
  : svtkm::filter::FilterField<DotProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(svtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("dotproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet DotProduct::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& primary,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  svtkm::cont::Field secondaryField;
  if (this->UseCoordinateSystemAsSecondaryField)
  {
    secondaryField = inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex());
  }
  else
  {
    secondaryField = inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation);
  }
  auto secondary =
    svtkm::filter::ApplyPolicyFieldOfType<svtkm::Vec<T, 3>>(secondaryField, policy, *this);

  svtkm::cont::ArrayHandle<T> output;
  this->Invoke(svtkm::worklet::DotProduct{}, primary, secondary, output);

  return CreateResult(inDataSet, output, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter

#endif
