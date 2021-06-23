//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtkm_m_filter_CellSetConnectivity_hxx
#define svtkm_m_filter_CellSetConnectivity_hxx

#include <svtkm/filter/CreateResult.h>
#include <svtkm/worklet/connectivities/CellSetConnectivity.h>

namespace svtkm
{
namespace filter
{

SVTKM_CONT CellSetConnectivity::CellSetConnectivity()
  : OutputFieldName("component")
{
}

template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet CellSetConnectivity::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<Policy> policy)
{
  svtkm::cont::ArrayHandle<svtkm::Id> component;

  svtkm::worklet::connectivity::CellSetConnectivity().Run(
    svtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy), component);

  return CreateResultFieldCell(input, component, this->GetOutputFieldName());
}
}
}

#endif //svtkm_m_filter_CellSetConnectivity_hxx
