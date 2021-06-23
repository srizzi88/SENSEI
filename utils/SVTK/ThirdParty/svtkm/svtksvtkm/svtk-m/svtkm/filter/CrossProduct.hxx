//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CrossProduct_hxx
#define svtk_m_filter_CrossProduct_hxx

#include <svtkm/cont/ArrayHandleCast.h>

#include <svtkm/worklet/DispatcherMapField.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT CrossProduct::CrossProduct()
  : svtkm::filter::FilterField<CrossProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(svtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet CrossProduct::DoExecute(
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

  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> output;
  this->Invoke(svtkm::worklet::CrossProduct{}, primary, secondary, output);

  return CreateResult(inDataSet, output, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter

#endif
