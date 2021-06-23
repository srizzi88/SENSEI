//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_GhostCellClassify_h
#define svtk_m_filter_GhostCellClassify_h

#include <svtkm/filter/FilterDataSet.h>

namespace svtkm
{
namespace filter
{

struct GhostCellClassifyPolicy : svtkm::filter::PolicyBase<GhostCellClassifyPolicy>
{
  using FieldTypeList = svtkm::List<svtkm::UInt8>;
};

class GhostCellClassify : public svtkm::filter::FilterDataSet<GhostCellClassify>
{
public:
  using SupportedTypes = svtkm::List<svtkm::UInt8>;

  SVTKM_CONT
  GhostCellClassify();

  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& inData,
                                          svtkm::filter::PolicyBase<Policy> policy);

  template <typename DerivedPolicy>
  SVTKM_CONT bool MapFieldOntoOutput(svtkm::cont::DataSet& result,
                                    const svtkm::cont::Field& field,
                                    const svtkm::filter::PolicyBase<DerivedPolicy>&)
  {
    result.AddField(field);
    return true;
  }

private:
};
}
} // namespace svtkm::filter

#include <svtkm/filter/GhostCellClassify.hxx>

#endif //svtk_m_filter_GhostCellClassify_h
