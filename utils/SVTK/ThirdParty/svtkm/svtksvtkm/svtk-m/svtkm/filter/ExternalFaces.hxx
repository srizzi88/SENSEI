//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExternalFaces_hxx
#define svtk_m_filter_ExternalFaces_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ExternalFaces::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //1. extract the cell set
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  svtkm::cont::CellSetExplicit<> outCellSet;

  if (cells.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    this->Worklet.Run(cells.Cast<svtkm::cont::CellSetStructured<3>>(),
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                      outCellSet);
  }
  else
  {
    this->Worklet.Run(svtkm::filter::ApplyPolicyCellSetUnstructured(cells, policy), outCellSet);
  }

  return this->GenerateOutput(input, outCellSet);
}
}
}

#endif
