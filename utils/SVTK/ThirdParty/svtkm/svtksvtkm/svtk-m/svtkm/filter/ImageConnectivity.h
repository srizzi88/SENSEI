//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ImageConnectivity_h
#define svtk_m_filter_ImageConnectivity_h

#include <svtkm/filter/FilterCell.h>
#include <svtkm/worklet/connectivities/ImageConnectivity.h>

/// \brief Groups connected points that have the same field value
///
///
/// The ImageConnectivity filter finds groups of points that have the same field value and are
/// connected together through their topology. Any point is considered to be connected to its Moore neighborhood:
/// 8 neighboring points for 2D and 27 neighboring points for 3D. As the name implies, ImageConnectivity only
/// works on data with a structured cell set. You will get an error if you use any other type of cell set.
/// The active field passed to the filter must be associated with the points.
/// The result of the filter is a point field of type svtkm::Id. Each entry in the point field will be a number that
/// identifies to which region it belongs. By default, this output point field is named “component”.
namespace svtkm
{
namespace filter
{
class ImageConnectivity : public svtkm::filter::FilterCell<ImageConnectivity>
{
public:
  using SupportedTypes = svtkm::TypeListScalarAll;

  SVTKM_CONT ImageConnectivity();

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMetadata,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>&);
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ImageConnectivity.hxx>

#endif //svtk_m_filter_ImageConnectivity_h
