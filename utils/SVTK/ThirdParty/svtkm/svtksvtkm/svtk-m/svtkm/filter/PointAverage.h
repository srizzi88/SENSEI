//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PointAverage_h
#define svtk_m_filter_PointAverage_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/filter/FilterCell.h>
#include <svtkm/worklet/PointAverage.h>

namespace svtkm
{
namespace filter
{
/// \brief Cell to Point interpolation filter.
///
/// PointAverage is a filter that transforms cell data (i.e., data
/// specified per cell) into point data (i.e., data specified at cell
/// points). The method of transformation is based on averaging the data
/// values of all cells using a particular point.
class SVTKM_ALWAYS_EXPORT PointAverage : public svtkm::filter::FilterCell<PointAverage>
{
public:
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::PointAverage Worklet;
};

#ifndef svtkm_filter_PointAverage_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(PointAverage);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/PointAverage.hxx>

#endif // svtk_m_filter_PointAverage_h
