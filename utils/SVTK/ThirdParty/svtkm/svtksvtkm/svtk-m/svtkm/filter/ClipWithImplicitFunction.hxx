//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ClipWithImplicitFunction_hxx
#define svtk_m_filter_ClipWithImplicitFunction_hxx

#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DynamicCellSet.h>

namespace svtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline svtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const svtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  svtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    svtkm::filter::ApplyPolicyCellSet(cells, policy), this->Function, inputCoords, this->Invert);

  // compute output coordinates
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData());
  svtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);

  //create the output data
  svtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);
  output.AddCoordinateSystem(outputCoords);

  return output;
}
}
} // end namespace svtkm::filter

#endif
