//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Tetrahedralize_h
#define svtk_m_filter_Tetrahedralize_h

#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Tetrahedralize.h>

namespace svtkm
{
namespace filter
{

class Tetrahedralize : public svtkm::filter::FilterDataSet<Tetrahedralize>
{
public:
  SVTKM_CONT
  Tetrahedralize();

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::Tetrahedralize Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Tetrahedralize.hxx>

#endif // svtk_m_filter_Tetrahedralize_h
