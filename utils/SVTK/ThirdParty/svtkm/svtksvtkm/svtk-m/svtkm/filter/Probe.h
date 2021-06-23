//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Probe_h
#define svtk_m_filter_Probe_h

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Probe.h>

namespace svtkm
{
namespace filter
{

class Probe : public svtkm::filter::FilterDataSet<Probe>
{
public:
  SVTKM_CONT
  void SetGeometry(const svtkm::cont::DataSet& geometry);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::cont::DataSet Geometry;
  svtkm::worklet::Probe Worklet;
};
}
} // svtkm::filter

#include <svtkm/filter/Probe.hxx>

#endif // svtk_m_filter_Probe_h
