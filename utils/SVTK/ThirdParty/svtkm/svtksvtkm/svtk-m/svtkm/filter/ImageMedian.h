//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ImageMedian_h
#define svtk_m_filter_ImageMedian_h

#include <svtkm/filter/FilterCell.h>

/// \brief Median algorithm for general image blur
///
/// The ImageMedian filter finds the median value for each pixel in an image.
/// Currently the algorithm has the following restrictions.
///   - Only supports a neighborhood of 5x5x1 or 3x3x1
///
/// This means that volumes are basically treated as an image stack
/// along the z axis
///
/// Default output field name is 'median'
namespace svtkm
{
namespace filter
{
class ImageMedian : public svtkm::filter::FilterField<ImageMedian>
{
  int Neighborhood = 1;

public:
  using SupportedTypes = svtkm::TypeListScalarAll;

  SVTKM_CONT ImageMedian();

  SVTKM_CONT void Perform3x3() { this->Neighborhood = 1; };
  SVTKM_CONT void Perform5x5() { this->Neighborhood = 2; };


  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMetadata,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>&);
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ImageMedian.hxx>

#endif //svtk_m_filter_ImageMedian_h
