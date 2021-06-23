//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CellAverage_hxx
#define svtk_m_filter_CellAverage_hxx

#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

namespace svtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet CellAverage::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& inField,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  svtkm::cont::DynamicCellSet cellSet = input.GetCellSet();

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  svtkm::cont::ArrayHandle<T> outArray;

  this->Invoke(this->Worklet, svtkm::filter::ApplyPolicyCellSet(cellSet, policy), inField, outArray);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = fieldMetadata.GetName();
  }

  return CreateResultFieldCell(input, outArray, outputName);
}
}
} // namespace svtkm::filter

#endif
