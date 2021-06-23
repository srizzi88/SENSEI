//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_SplitSharpEdges_hxx
#define svtk_m_filter_SplitSharpEdges_hxx

#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT SplitSharpEdges::SplitSharpEdges()
  : svtkm::filter::FilterDataSetWithField<SplitSharpEdges>()
  , FeatureAngle(30.0)
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet SplitSharpEdges::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& svtkmNotUsed(fieldMeta),
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // Get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  svtkm::cont::ArrayHandle<svtkm::Vec3f> newCoords;
  svtkm::cont::CellSetExplicit<> newCellset;

  this->Worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                    this->FeatureAngle,
                    field,
                    input.GetCoordinateSystem().GetData(),
                    newCoords,
                    newCellset);

  svtkm::cont::DataSet output;
  output.SetCellSet(newCellset);
  output.AddCoordinateSystem(
    svtkm::cont::CoordinateSystem(input.GetCoordinateSystem().GetName(), newCoords));
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool SplitSharpEdges::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    // We copy the input handle to the result dataset, reusing the metadata
    svtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessPointField(input);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else
  {
    return false;
  }
}
}
}
#endif
