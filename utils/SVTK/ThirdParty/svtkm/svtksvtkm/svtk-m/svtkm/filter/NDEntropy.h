//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_NDEntropy_h
#define svtk_m_filter_NDEntropy_h

#include <svtkm/filter/FilterDataSet.h>

namespace svtkm
{
namespace filter
{
/// \brief Calculate the entropy of input N-Dims fields
///
/// This filter calculate the entropy of input N-Dims fields.
///
class NDEntropy : public svtkm::filter::FilterDataSet<NDEntropy>
{
public:
  SVTKM_CONT
  NDEntropy();

  SVTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, svtkm::Id numOfBins);

  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& inData,
                                          svtkm::filter::PolicyBase<Policy> policy);

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  std::vector<svtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/NDEntropy.hxx>

#endif //svtk_m_filter_NDEntropy_h
