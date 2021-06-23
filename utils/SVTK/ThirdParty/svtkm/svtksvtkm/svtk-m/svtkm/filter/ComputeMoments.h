//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=======================================================================
#ifndef svtk_m_filter_ComputeMoments_h
#define svtk_m_filter_ComputeMoments_h

#include <svtkm/filter/FilterCell.h>


namespace svtkm
{
namespace filter
{
class ComputeMoments : public svtkm::filter::FilterCell<ComputeMoments>
{
public:
  SVTKM_CONT ComputeMoments();

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMetadata,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>&);

  SVTKM_CONT void SetRadius(double _radius) { this->Radius = _radius; }

  SVTKM_CONT void SetSpacing(svtkm::Vec3f _spacing) { this->Spacing = _spacing; }

  SVTKM_CONT void SetOrder(svtkm::Int32 _order) { this->Order = _order; }

private:
  double Radius = 1;
  svtkm::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  svtkm::Int32 Order = 0;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ComputeMoments.hxx>

#endif //svtk_m_filter_ComputeMoments_h
