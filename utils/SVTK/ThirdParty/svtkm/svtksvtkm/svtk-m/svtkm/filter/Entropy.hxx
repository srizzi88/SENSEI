//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Entropy_hxx
#define svtk_m_filter_Entropy_hxx

#include <svtkm/worklet/FieldEntropy.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Entropy::Entropy()
  : NumberOfBins(10)
{
  this->SetOutputFieldName("entropy");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Entropy::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  svtkm::worklet::FieldEntropy worklet;

  svtkm::Float64 e = worklet.Run(field, this->NumberOfBins);

  //the entropy vector only contain one element, the entorpy of the input field
  svtkm::cont::ArrayHandle<svtkm::Float64> entropy;
  entropy.Allocate(1);
  entropy.GetPortalControl().Set(0, e);

  return CreateResult(inDataSet, entropy, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter

#endif
