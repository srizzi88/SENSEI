//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ThresholdPoints_h
#define svtk_m_filter_ThresholdPoints_h

#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/worklet/ThresholdPoints.h>

namespace svtkm
{
namespace filter
{

class ThresholdPoints : public svtkm::filter::FilterDataSetWithField<ThresholdPoints>
{
public:
  using SupportedTypes = svtkm::TypeListScalarAll;

  SVTKM_CONT
  ThresholdPoints();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  SVTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  SVTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  SVTKM_CONT
  svtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  SVTKM_CONT
  void SetLowerThreshold(svtkm::Float64 value) { this->LowerValue = value; }

  SVTKM_CONT
  svtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }
  SVTKM_CONT
  void SetUpperThreshold(svtkm::Float64 value) { this->UpperValue = value; }

  SVTKM_CONT
  void SetThresholdBelow(const svtkm::Float64 value);
  SVTKM_CONT
  void SetThresholdAbove(const svtkm::Float64 value);
  SVTKM_CONT
  void SetThresholdBetween(const svtkm::Float64 value1, const svtkm::Float64 value2);

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
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  double LowerValue;
  double UpperValue;
  int ThresholdType;

  bool CompactPoints;
  svtkm::filter::CleanGrid Compactor;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ThresholdPoints.hxx>

#endif // svtk_m_filter_ThresholdPoints_h
