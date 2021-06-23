//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_MaskPoints_h
#define svtk_m_filter_MaskPoints_h

#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/MaskPoints.h>

namespace svtkm
{
namespace filter
{

/// \brief Subselect points using a stride
///
/// Extract only every Nth point where N is equal to a stride value
class MaskPoints : public svtkm::filter::FilterDataSet<MaskPoints>
{
public:
  SVTKM_CONT
  MaskPoints();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  SVTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  SVTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  SVTKM_CONT
  svtkm::Id GetStride() const { return this->Stride; }
  SVTKM_CONT
  void SetStride(svtkm::Id stride) { this->Stride = stride; }

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::Id Stride;
  bool CompactPoints;
  svtkm::filter::CleanGrid Compactor;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/MaskPoints.hxx>

#endif // svtk_m_filter_MaskPoints_h
