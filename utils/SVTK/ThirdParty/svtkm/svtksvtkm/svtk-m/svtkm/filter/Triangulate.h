//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Triangulate_h
#define svtk_m_filter_Triangulate_h

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Triangulate.h>

namespace svtkm
{
namespace filter
{

class Triangulate : public svtkm::filter::FilterDataSet<Triangulate>
{
public:
  SVTKM_CONT
  Triangulate();

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::Triangulate Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Triangulate.hxx>

#endif // svtk_m_filter_Triangulate_h
