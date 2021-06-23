//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_NDHistogram_h
#define svtk_m_filter_NDHistogram_h

#include <svtkm/filter/FilterDataSet.h>

namespace svtkm
{
namespace filter
{
/// \brief Generate a N-Dims histogram from input fields
///
/// This filter takes a data set and with target fields and bins defined,
/// it would generate a N-Dims histogram from input fields. The result is stored
/// in a field named as "Frequency". This filed contains all the frequencies of
/// the N-Dims histogram in sparse representation. That being said, the result
/// field does not store 0 frequency bins. Meanwhile all input fields now
/// would have the same length and store bin ids instead.
/// E.g. (FieldA[i], FieldB[i], FieldC[i], Frequency[i]) is a bin in the histogram.
/// The first three numbers are binIDs for FieldA, FieldB and FieldC. Frequency[i] stores
/// the frequency for this bin (FieldA[i], FieldB[i], FieldC[i]).
///
class NDHistogram : public svtkm::filter::FilterDataSet<NDHistogram>
{
public:
  SVTKM_CONT
  NDHistogram();

  SVTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, svtkm::Id numOfBins);

  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& inData,
                                          svtkm::filter::PolicyBase<Policy> policy);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  SVTKM_CONT
  svtkm::Float64 GetBinDelta(size_t fieldIdx);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  SVTKM_CONT
  svtkm::Range GetDataRange(size_t fieldIdx);

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  std::vector<svtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
  std::vector<svtkm::Float64> BinDeltas;
  std::vector<svtkm::Range> DataRanges; //Min Max of the field
};
}
} // namespace svtkm::filter

#include <svtkm/filter/NDHistogram.hxx>

#endif //svtk_m_filter_NDHistogram_h
