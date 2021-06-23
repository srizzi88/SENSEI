//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Triangulate_hxx
#define svtk_m_filter_Triangulate_hxx

namespace
{

class DeduceCellSet
{
  mutable svtkm::worklet::Triangulate Worklet;
  svtkm::cont::CellSetSingleType<>& OutCellSet;

public:
  DeduceCellSet(svtkm::worklet::Triangulate worklet, svtkm::cont::CellSetSingleType<>& outCellSet)
    : Worklet(worklet)
    , OutCellSet(outCellSet)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& svtkmNotUsed(cellset)) const
  {
  }
};
template <>
void DeduceCellSet::operator()(const svtkm::cont::CellSetExplicit<>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
template <>
void DeduceCellSet::operator()(const svtkm::cont::CellSetStructured<2>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
template <>
void DeduceCellSet::operator()(const svtkm::cont::CellSetStructured<3>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
}

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Triangulate::Triangulate()
  : svtkm::filter::FilterDataSet<Triangulate>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Triangulate::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  svtkm::cont::CellSetSingleType<> outCellSet;
  DeduceCellSet triangulate(this->Worklet, outCellSet);

  svtkm::cont::CastAndCall(svtkm::filter::ApplyPolicyCellSet(cells, policy), triangulate);

  // create the output dataset
  svtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Triangulate::DoMapField(svtkm::cont::DataSet& result,
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
