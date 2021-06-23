//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractStructured_hxx
#define svtk_m_filter_ExtractStructured_hxx

namespace svtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ExtractStructured::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coordinates = input.GetCoordinateSystem();

  auto cellset = this->Worklet.Run(svtkm::filter::ApplyPolicyCellSetStructured(cells, policy),
                                   this->VOI,
                                   this->SampleRate,
                                   this->IncludeBoundary,
                                   this->IncludeOffset);

  auto coords = this->Worklet.MapCoordinates(coordinates);
  svtkm::cont::CoordinateSystem outputCoordinates(coordinates.GetName(), coords);

  svtkm::cont::DataSet output;
  output.SetCellSet(svtkm::cont::DynamicCellSet(cellset));
  output.AddCoordinateSystem(outputCoordinates);
  return output;
}
}
}

#endif
