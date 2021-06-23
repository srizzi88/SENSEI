//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ImageConnectivity_hxx
#define svtk_m_filter_ImageConnectivity_hxx

namespace svtkm
{
namespace filter
{

SVTKM_CONT ImageConnectivity::ImageConnectivity()
{
  this->SetOutputFieldName("component");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ImageConnectivity::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw svtkm::cont::ErrorBadValue("Active field for ImageConnectivity must be a point field.");
  }

  svtkm::cont::ArrayHandle<svtkm::Id> component;

  svtkm::worklet::connectivity::ImageConnectivity().Run(
    svtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy), field, component);

  auto result = CreateResult(input, component, this->GetOutputFieldName(), fieldMetadata);
  return result;
}
}
}

#endif
