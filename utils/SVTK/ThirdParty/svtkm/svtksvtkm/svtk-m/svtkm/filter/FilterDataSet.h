//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_DataSetFilter_h
#define svtk_m_filter_DataSetFilter_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <svtkm/filter/Filter.h>
#include <svtkm/filter/PolicyBase.h>

namespace svtkm
{
namespace filter
{

template <class Derived>
class FilterDataSet : public svtkm::filter::Filter<Derived>
{
public:
  SVTKM_CONT
  FilterDataSet();

  SVTKM_CONT
  ~FilterDataSet();

  SVTKM_CONT
  void SetActiveCoordinateSystem(svtkm::Id index) { this->CoordinateSystemIndex = index; }

  SVTKM_CONT
  svtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }

  /// These are provided to satisfy the Filter API requirements.

  //From the field we can extract the association component
  // Association::ANY -> unable to map
  // Association::WHOLE_MESH -> (I think this is points)
  // Association::POINTS -> map using point mapping
  // Association::CELL_SET -> how do we map this?
  template <typename DerivedPolicy>
  SVTKM_CONT bool MapFieldOntoOutput(svtkm::cont::DataSet& result,
                                    const svtkm::cont::Field& field,
                                    svtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet PrepareForExecution(const svtkm::cont::DataSet& input,
                                                    svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::Id CoordinateSystemIndex;

  friend class svtkm::filter::Filter<Derived>;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/FilterDataSet.hxx>

#endif // svtk_m_filter_DataSetFilter_h
