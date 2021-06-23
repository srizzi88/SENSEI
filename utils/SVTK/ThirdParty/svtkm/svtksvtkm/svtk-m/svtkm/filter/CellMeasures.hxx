//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CellMeasures_hxx
#define svtk_m_filter_CellMeasures_hxx

#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename IntegrationType>
inline SVTKM_CONT CellMeasures<IntegrationType>::CellMeasures()
  : svtkm::filter::FilterCell<CellMeasures<IntegrationType>>()
{
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
template <typename IntegrationType>
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet CellMeasures<IntegrationType>::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& points,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("CellMeasures expects point field input.");
  }

  const auto& cellset = input.GetCellSet();
  svtkm::cont::ArrayHandle<T> outArray;

  this->Invoke(svtkm::worklet::CellMeasure<IntegrationType>{},
               svtkm::filter::ApplyPolicyCellSet(cellset, policy),
               points,
               outArray);

  std::string outputName = this->GetCellMeasureName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = "measure";
  }
  return CreateResultFieldCell(input, outArray, outputName);
}
}
} // namespace svtkm::filter

#endif
