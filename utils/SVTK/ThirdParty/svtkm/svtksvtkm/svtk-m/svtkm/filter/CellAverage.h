//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CellAverage_h
#define svtk_m_filter_CellAverage_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/filter/FilterCell.h>
#include <svtkm/worklet/CellAverage.h>

namespace svtkm
{
namespace filter
{
/// \brief  Point to cell interpolation filter.
///
/// CellAverage is a filter that transforms point data (i.e., data
/// specified at cell points) into cell data (i.e., data specified per cell).
/// The method of transformation is based on averaging the data
/// values of all points used by particular cell.
///
class SVTKM_ALWAYS_EXPORT CellAverage : public svtkm::filter::FilterCell<CellAverage>
{
public:
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::CellAverage Worklet;
};

#ifndef svtkm_filter_CellAverage_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(CellAverage);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/CellAverage.hxx>

#endif // svtk_m_filter_CellAverage_h
