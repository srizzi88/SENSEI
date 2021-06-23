//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_VectorMagnitude_hxx
#define svtk_m_filter_VectorMagnitude_hxx

#include <svtkm/Math.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet VectorMagnitude::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  using ReturnType = typename ::svtkm::detail::FloatingPointReturnType<T>::Type;
  svtkm::cont::ArrayHandle<ReturnType> outArray;

  this->Invoke(this->Worklet, field, outArray);

  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace svtkm::filter
#endif
