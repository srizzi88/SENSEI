//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_NDEntropy_hxx
#define svtk_m_filter_NDEntropy_hxx

#include <svtkm/cont/DataSet.h>
#include <svtkm/worklet/NDimsEntropy.h>

namespace svtkm
{
namespace filter
{

inline SVTKM_CONT NDEntropy::NDEntropy()
{
}

void NDEntropy::AddFieldAndBin(const std::string& fieldName, svtkm::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

template <typename Policy>
inline SVTKM_CONT svtkm::cont::DataSet NDEntropy::DoExecute(
  const svtkm::cont::DataSet& inData,
  svtkm::filter::PolicyBase<Policy> svtkmNotUsed(policy))
{
  svtkm::worklet::NDimsEntropy ndEntropy;
  ndEntropy.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < FieldNames.size(); i++)
  {
    ndEntropy.AddField(inData.GetField(FieldNames[i]).GetData(), NumOfBins[i]);
  }

  // Run worklet to calculate multi-variate entropy
  svtkm::cont::ArrayHandle<svtkm::Float64> entropyHandle;
  svtkm::Float64 entropy = ndEntropy.Run();

  entropyHandle.Allocate(1);
  entropyHandle.GetPortalControl().Set(0, entropy);


  svtkm::cont::DataSet outputData;
  outputData.AddField(svtkm::cont::make_FieldPoint("Entropy", entropyHandle));
  return outputData;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool NDEntropy::DoMapField(svtkm::cont::DataSet&,
                                            const svtkm::cont::ArrayHandle<T, StorageType>&,
                                            const svtkm::filter::FieldMetadata&,
                                            svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
}
#endif
