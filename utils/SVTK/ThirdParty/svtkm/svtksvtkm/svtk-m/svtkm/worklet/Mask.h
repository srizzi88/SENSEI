//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_Mask_h
#define svtkm_m_worklet_Mask_h

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class Mask
{
public:
  template <typename CellSetType>
  svtkm::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet, const svtkm::Id stride)
  {
    using OutputType = svtkm::cont::CellSetPermutation<CellSetType>;

    svtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    svtkm::Id numberOfSampledCells = numberOfInputCells / stride;
    svtkm::cont::ArrayHandleCounting<svtkm::Id> strideArray(0, stride, numberOfSampledCells);

    svtkm::cont::ArrayCopy(strideArray, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = svtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    svtkm::cont::ArrayHandle<ValueType> result;
    svtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> ValidCellIds;
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_Mask_h
