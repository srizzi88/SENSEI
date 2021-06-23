//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_NDHistogram_hxx
#define svtk_m_filter_NDHistogram_hxx

#include <vector>
#include <svtkm/cont/DataSet.h>
#include <svtkm/worklet/NDimsHistogram.h>

namespace svtkm
{
namespace filter
{

inline SVTKM_CONT NDHistogram::NDHistogram()
{
}

void NDHistogram::AddFieldAndBin(const std::string& fieldName, svtkm::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

svtkm::Float64 NDHistogram::GetBinDelta(size_t fieldIdx)
{
  return BinDeltas[fieldIdx];
}

svtkm::Range NDHistogram::GetDataRange(size_t fieldIdx)
{
  return DataRanges[fieldIdx];
}

template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet NDHistogram::DoExecute(const svtkm::cont::DataSet& inData,
                                                            svtkm::filter::PolicyBase<Policy> policy)
{
  svtkm::worklet::NDimsHistogram ndHistogram;

  // Set the number of data points
  ndHistogram.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < this->FieldNames.size(); i++)
  {
    svtkm::Range rangeField;
    svtkm::Float64 deltaField;
    ndHistogram.AddField(
      svtkm::filter::ApplyPolicyFieldNotActive(inData.GetField(this->FieldNames[i]), policy),
      this->NumOfBins[i],
      rangeField,
      deltaField);
    DataRanges.push_back(rangeField);
    BinDeltas.push_back(deltaField);
  }

  std::vector<svtkm::cont::ArrayHandle<svtkm::Id>> binIds;
  svtkm::cont::ArrayHandle<svtkm::Id> freqs;
  ndHistogram.Run(binIds, freqs);

  svtkm::cont::DataSet outputData;
  for (size_t i = 0; i < binIds.size(); i++)
  {
    outputData.AddField(svtkm::cont::make_FieldPoint(this->FieldNames[i], binIds[i]));
  }
  outputData.AddField(svtkm::cont::make_FieldPoint("Frequency", freqs));

  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool NDHistogram::DoMapField(svtkm::cont::DataSet&,
                                              const svtkm::cont::ArrayHandle<T, StorageType>&,
                                              const svtkm::filter::FieldMetadata&,
                                              svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
}
#endif
