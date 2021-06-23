//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Mask_h
#define svtk_m_filter_Mask_h

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Mask.h>

namespace svtkm
{
namespace filter
{
/// \brief Subselect cells using a stride
///
/// Extract only every Nth cell where N is equal to a stride value
class Mask : public svtkm::filter::FilterDataSet<Mask>
{
public:
  SVTKM_CONT
  Mask();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  SVTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  SVTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // Set the stride of the subsample
  SVTKM_CONT
  svtkm::Id GetStride() const { return this->Stride; }
  SVTKM_CONT
  void SetStride(svtkm::Id& stride) { this->Stride = stride; }

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
  svtkm::worklet::Mask Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Mask.hxx>

#endif // svtk_m_filter_Mask_h
