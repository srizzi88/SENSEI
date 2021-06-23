//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Threshold_h
#define svtk_m_filter_Threshold_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/worklet/Threshold.h>

namespace svtkm
{
namespace filter
{

/// \brief Extracts cells where scalar value in cell satisfies threshold criterion
///
/// Extracts all cells from any dataset type that
/// satisfy a threshold criterion. A cell satisfies the criterion if the
/// scalar value of every point or cell satisfies the criterion. The
/// criterion takes the form of between two values. The output of this
/// filter is an permutation of the input dataset.
///
/// You can threshold either on point or cell fields
class SVTKM_ALWAYS_EXPORT Threshold : public svtkm::filter::FilterDataSetWithField<Threshold>
{
public:
  using SupportedTypes = svtkm::TypeListScalarAll;

  SVTKM_CONT
  void SetLowerThreshold(svtkm::Float64 value) { this->LowerValue = value; }
  SVTKM_CONT
  void SetUpperThreshold(svtkm::Float64 value) { this->UpperValue = value; }

  SVTKM_CONT
  svtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  SVTKM_CONT
  svtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy>)
  {
    if (fieldMeta.IsPointField())
    {
      //we copy the input handle to the result dataset, reusing the metadata
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
    else if (fieldMeta.IsCellField())
    {
      svtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input);
      result.AddField(fieldMeta.AsField(out));
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  double LowerValue = 0;
  double UpperValue = 0;
  svtkm::worklet::Threshold Worklet;
};

#ifndef svtkm_filter_Threshold_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(Threshold);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/Threshold.hxx>

#endif // svtk_m_filter_Threshold_h
