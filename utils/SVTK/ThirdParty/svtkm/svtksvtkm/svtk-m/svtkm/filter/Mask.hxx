//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Mask_hxx
#define svtk_m_filter_Mask_hxx

namespace
{

struct CallWorklet
{
  svtkm::Id Stride;
  svtkm::cont::DynamicCellSet& Output;
  svtkm::worklet::Mask& Worklet;

  CallWorklet(svtkm::Id stride, svtkm::cont::DynamicCellSet& output, svtkm::worklet::Mask& worklet)
    : Stride(stride)
    , Output(output)
    , Worklet(worklet)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cells) const
  {
    this->Output = this->Worklet.Run(cells, this->Stride);
  }
};

} // end anon namespace

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Mask::Mask()
  : svtkm::filter::FilterDataSet<Mask>()
  , Stride(1)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Mask::DoExecute(const svtkm::cont::DataSet& input,
                                                     svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  svtkm::cont::DynamicCellSet cellOut;
  CallWorklet workletCaller(this->Stride, cellOut, this->Worklet);
  svtkm::filter::ApplyPolicyCellSet(cells, policy).CastAndCall(workletCaller);

  // create the output dataset
  svtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(cellOut);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Mask::DoMapField(svtkm::cont::DataSet& result,
                                       const svtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const svtkm::filter::FieldMetadata& fieldMeta,
                                       svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::Field output;

  if (fieldMeta.IsPointField())
  {
    output = fieldMeta.AsField(input); // pass through
  }
  else if (fieldMeta.IsCellField())
  {
    output = fieldMeta.AsField(this->Worklet.ProcessCellField(input));
  }
  else
  {
    return false;
  }

  result.AddField(output);
  return true;
}
}
}
#endif
