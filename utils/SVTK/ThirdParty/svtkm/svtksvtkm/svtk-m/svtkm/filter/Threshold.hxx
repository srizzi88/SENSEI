//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Threshold_hxx
#define svtk_m_filter_Threshold_hxx

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DynamicCellSet.h>

namespace
{

class ThresholdRange
{
public:
  SVTKM_CONT
  ThresholdRange(const svtkm::Float64& lower, const svtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  SVTKM_EXEC bool operator()(const T& value) const
  {

    return value >= static_cast<T>(this->Lower) && value <= static_cast<T>(this->Upper);
  }

  //Needed to work with ArrayHandleVirtual
  template <typename PortalType>
  SVTKM_EXEC bool operator()(
    const svtkm::internal::ArrayPortalValueReference<PortalType>& value) const
  {
    using T = typename PortalType::ValueType;
    return value.Get() >= static_cast<T>(this->Lower) && value.Get() <= static_cast<T>(this->Upper);
  }

private:
  svtkm::Float64 Lower;
  svtkm::Float64 Upper;
};

} // end anon namespace

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Threshold::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  svtkm::cont::DynamicCellSet cellOut = this->Worklet.Run(
    svtkm::filter::ApplyPolicyCellSet(cells, policy), field, fieldMeta.GetAssociation(), predicate);

  svtkm::cont::DataSet output;
  output.SetCellSet(cellOut);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  return output;
}
}
}
#endif
