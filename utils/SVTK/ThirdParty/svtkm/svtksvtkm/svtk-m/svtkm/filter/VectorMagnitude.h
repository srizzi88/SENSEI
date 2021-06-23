//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_VectorMagnitude_h
#define svtk_m_filter_VectorMagnitude_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/Magnitude.h>

namespace svtkm
{
namespace filter
{

class SVTKM_ALWAYS_EXPORT VectorMagnitude : public svtkm::filter::FilterField<VectorMagnitude>
{
public:
  //currently the VectorMagnitude filter only works on vector data.
  using SupportedTypes = svtkm::TypeListVecCommon;

  SVTKM_FILTER_EXPORT
  VectorMagnitude();

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::worklet::Magnitude Worklet;
};
#ifndef svtkm_filter_VectorMagnitude_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(VectorMagnitude);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/VectorMagnitude.hxx>

#endif // svtk_m_filter_VectorMagnitude_h
