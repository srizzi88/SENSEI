//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtkm_m_filter_CleanGrid_hxx
#define svtkm_m_filter_CleanGrid_hxx

#include <svtkm/worklet/CellDeepCopy.h>
#include <svtkm/worklet/RemoveUnusedPoints.h>

#include <vector>

namespace svtkm
{
namespace filter
{

template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet CleanGrid::DoExecute(const svtkm::cont::DataSet& inData,
                                                          svtkm::filter::PolicyBase<Policy> policy)
{
  using CellSetType = svtkm::cont::CellSetExplicit<>;

  CellSetType outputCellSet;
  // Do a deep copy of the cells to new CellSetExplicit structures
  const svtkm::cont::DynamicCellSet& inCellSet = inData.GetCellSet();
  if (inCellSet.IsType<CellSetType>())
  {
    // Is expected type, do a shallow copy
    outputCellSet = inCellSet.Cast<CellSetType>();
  }
  else
  { // Clean the grid
    auto deducedCellSet = svtkm::filter::ApplyPolicyCellSet(inCellSet, policy);
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;

    this->Invoke(worklet::CellDeepCopy::CountCellPoints{}, deducedCellSet, numIndices);

    svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
    svtkm::cont::ArrayHandle<svtkm::Id> offsets;
    svtkm::Id connectivitySize;
    svtkm::cont::ConvertNumIndicesToOffsets(numIndices, offsets, connectivitySize);
    numIndices.ReleaseResourcesExecution();

    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    connectivity.Allocate(connectivitySize);

    auto offsetsTrim =
      svtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

    this->Invoke(worklet::CellDeepCopy::PassCellStructure{},
                 deducedCellSet,
                 shapes,
                 svtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsetsTrim));
    shapes.ReleaseResourcesExecution();
    offsets.ReleaseResourcesExecution();
    connectivity.ReleaseResourcesExecution();

    outputCellSet.Fill(deducedCellSet.GetNumberOfPoints(), shapes, connectivity, offsets);

    //Release the input grid from the execution space
    deducedCellSet.ReleaseResourcesExecution();
  }

  return this->GenerateOutput(inData, outputCellSet);
}
}
}

#endif
