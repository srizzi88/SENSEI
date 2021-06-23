//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractPoints_hxx
#define svtk_m_filter_ExtractPoints_hxx

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT ExtractPoints::ExtractPoints()
  : svtkm::filter::FilterDataSet<ExtractPoints>()
  , ExtractInside(true)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline svtkm::cont::DataSet ExtractPoints::DoExecute(const svtkm::cont::DataSet& input,
                                                    svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set and coordinates
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  svtkm::cont::CellSetSingleType<> outCellSet;
  svtkm::worklet::ExtractPoints worklet;

  outCellSet = worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                           coords.GetData(),
                           this->Function,
                           this->ExtractInside);

  // create the output dataset
  svtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(false);
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool ExtractPoints::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy);
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
  }

  // cell data does not apply
  return false;
}
}
}

#endif
