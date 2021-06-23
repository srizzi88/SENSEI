//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ClipWithField_hxx
#define svtk_m_filter_ClipWithField_hxx

#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>

namespace svtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ClipWithField::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  //get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const svtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  svtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    svtkm::filter::ApplyPolicyCellSet(cells, policy), field, this->ClipValue, this->Invert);

  //create the output data
  svtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);

  // Compute the new boundary points and add them to the output:
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData());
  svtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);
  output.AddCoordinateSystem(outputCoords);
  return output;
}
}
} // end namespace svtkm::filter

#endif
