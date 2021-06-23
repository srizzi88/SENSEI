//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Tetrahedralize_hxx
#define svtk_m_filter_Tetrahedralize_hxx

namespace
{
struct DeduceCellSet
{
  template <typename CellSetType>
  void operator()(const CellSetType& cellset,
                  svtkm::worklet::Tetrahedralize& worklet,
                  svtkm::cont::CellSetSingleType<>& outCellSet) const
  {
    outCellSet = worklet.Run(cellset);
  }
};
}

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Tetrahedralize::Tetrahedralize()
  : svtkm::filter::FilterDataSet<Tetrahedralize>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Tetrahedralize::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  svtkm::cont::CellSetSingleType<> outCellSet;
  svtkm::cont::CastAndCall(
    svtkm::filter::ApplyPolicyCellSet(cells, policy), DeduceCellSet{}, this->Worklet, outCellSet);

  // create the output dataset
  svtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Tetrahedralize::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    result.AddField(fieldMeta.AsField(input));
    return true;
  }

  // cell data must be scattered to the cells created per input cell
  if (fieldMeta.IsCellField())
  {
    svtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}
}
}
#endif
