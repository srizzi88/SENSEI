//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_VertexClustering_hxx
#define svtk_m_filter_VertexClustering_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT VertexClustering::VertexClustering()
  : svtkm::filter::FilterDataSet<VertexClustering>()
  , NumberOfDivisions(256, 256, 256)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet VertexClustering::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // todo this code needs to obey the policy for what storage types
  // the output should use
  //need to compute bounds first
  svtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  svtkm::cont::DataSet outDataSet =
    this->Worklet.Run(svtkm::filter::ApplyPolicyCellSetUnstructured(input.GetCellSet(), policy),
                      input.GetCoordinateSystem(),
                      bounds,
                      this->GetNumberOfDivisions());

  return outDataSet;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool VertexClustering::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<T> fieldArray;

  if (fieldMeta.IsPointField())
  {
    fieldArray = this->Worklet.ProcessPointField(input);
  }
  else if (fieldMeta.IsCellField())
  {
    fieldArray = this->Worklet.ProcessCellField(input);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(fieldArray));

  return true;
}
}
}
#endif
