//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_MaskPoints_hxx
#define svtk_m_filter_MaskPoints_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT MaskPoints::MaskPoints()
  : svtkm::filter::FilterDataSet<MaskPoints>()
  , Stride(1)
  , CompactPoints(true)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet MaskPoints::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  // run the worklet on the cell set and input field
  svtkm::cont::CellSetSingleType<> outCellSet;
  svtkm::worklet::MaskPoints worklet;

  outCellSet = worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy), this->Stride);

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
inline SVTKM_CONT bool MaskPoints::DoMapField(svtkm::cont::DataSet& result,
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
