//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Probe_hxx
#define svtk_m_filter_Probe_hxx

namespace svtkm
{
namespace filter
{

SVTKM_CONT
inline void Probe::SetGeometry(const svtkm::cont::DataSet& geometry)
{
  this->Geometry = svtkm::cont::DataSet();
  this->Geometry.SetCellSet(geometry.GetCellSet());
  this->Geometry.AddCoordinateSystem(geometry.GetCoordinateSystem());
}

template <typename DerivedPolicy>
SVTKM_CONT inline svtkm::cont::DataSet Probe::DoExecute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  this->Worklet.Run(svtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy),
                    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                    this->Geometry.GetCoordinateSystem().GetData());

  auto output = this->Geometry;
  auto hpf = this->Worklet.GetHiddenPointsField();
  auto hcf = this->Worklet.GetHiddenCellsField(
    svtkm::filter::ApplyPolicyCellSet(output.GetCellSet(), policy));

  output.AddField(svtkm::cont::make_FieldPoint("HIDDEN", hpf));
  output.AddField(svtkm::cont::make_FieldCell("HIDDEN", hcf));

  return output;
}

template <typename T, typename StorageType, typename DerivedPolicy>
SVTKM_CONT inline bool Probe::DoMapField(svtkm::cont::DataSet& result,
                                        const svtkm::cont::ArrayHandle<T, StorageType>& input,
                                        const svtkm::filter::FieldMetadata& fieldMeta,
                                        svtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    auto fieldArray =
      this->Worklet.ProcessPointField(input, typename DerivedPolicy::AllCellSetList());
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    auto fieldArray = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

  return false;
}
}
} // svtkm::filter
#endif
