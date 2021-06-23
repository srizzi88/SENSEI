//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_NDimsHistMarginalization_h
#define svtk_m_worklet_NDimsHistMarginalization_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>
#include <svtkm/worklet/histogram/MarginalizeNDHistogram.h>
#include <svtkm/worklet/histogram/MarginalizeNDHistogram.h>
#include <svtkm/worklet/histogram/MarginalizeNDHistogram.h>

#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace worklet
{


class NDimsHistMarginalization
{
public:
  // Execute the histogram (conditional) marginalization,
  //   given the multi-variable histogram(binId, freqIn)
  //   , marginalVariable and marginal condition
  // Input arguments:
  //   binId, freqsIn: input ND-histogram in the fashion of sparse representation
  //                   (definition of binId and frqIn please refer to NDimsHistogram.h),
  //                   (binId.size() is the number of variables)
  //   numberOfBins: number of bins of each variable (length of numberOfBins must be the same as binId.size() )
  //   marginalVariables: length is the same as number of variables.
  //                      1 indicates marginal variable, otherwise 0.
  //   conditionFunc: The Condition function for non-marginal variable.
  //                  This func takes two arguments (svtkm::Id var, svtkm::Id binId) and return bool
  //                  var is index of variable and binId is bin index in the variable var
  //                  return true indicates considering this bin into final marginal histogram
  //                  more details can refer to example in UnitTestNDimsHistMarginalization.cxx
  //   marginalBinId, marginalFreqs: return marginalized histogram in the fashion of sparse representation
  //                                 the definition is the same as (binId and freqsIn)
  template <typename BinaryCompare>
  void Run(const std::vector<svtkm::cont::ArrayHandle<svtkm::Id>>& binId,
           svtkm::cont::ArrayHandle<svtkm::Id>& freqsIn,
           svtkm::cont::ArrayHandle<svtkm::Id>& numberOfBins,
           svtkm::cont::ArrayHandle<bool>& marginalVariables,
           BinaryCompare conditionFunc,
           std::vector<svtkm::cont::ArrayHandle<svtkm::Id>>& marginalBinId,
           svtkm::cont::ArrayHandle<svtkm::Id>& marginalFreqs)
  {
    //total variables
    svtkm::Id numOfVariable = static_cast<svtkm::Id>(binId.size());

    const svtkm::Id numberOfValues = freqsIn.GetNumberOfValues();
    svtkm::cont::ArrayHandleConstant<svtkm::Id> constant0Array(0, numberOfValues);
    svtkm::cont::ArrayHandle<svtkm::Id> bin1DIndex;
    svtkm::cont::ArrayCopy(constant0Array, bin1DIndex);

    svtkm::cont::ArrayHandle<svtkm::Id> freqs;
    svtkm::cont::ArrayCopy(freqsIn, freqs);
    svtkm::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.GetPortalConstControl();
    const auto numBinsPortal = numberOfBins.GetPortalConstControl();
    for (svtkm::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const svtkm::Id nFieldBins = numBinsPortal.Get(i);
        svtkm::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::To1DIndex> to1DIndexDispatcher(
          binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
      else
      { //non-marginal variable
        // Worklet to set the frequency of entities which does not meet the condition
        // to 0 on non-marginal variables
        svtkm::worklet::histogram::ConditionalFreq<BinaryCompare> conditionalFreqWorklet{
          conditionFunc
        };
        conditionalFreqWorklet.setVar(i);
        svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::ConditionalFreq<BinaryCompare>>
          cfDispatcher(conditionalFreqWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        cfDispatcher.Invoke(binId[vecIndex], freqs, freqs);
      }
    }


    // Sort the freq array for counting by key(1DIndex)
    svtkm::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin (this get a nonSparse representation)
    svtkm::cont::ArrayHandle<svtkm::Id> nonSparseMarginalFreqs;
    svtkm::cont::Algorithm::ReduceByKey(
      bin1DIndex, freqs, bin1DIndex, nonSparseMarginalFreqs, svtkm::Add());

    // Convert to sparse representation(remove all zero freqncy entities)
    svtkm::cont::ArrayHandle<svtkm::Id> sparseMarginal1DBinId;
    svtkm::cont::Algorithm::CopyIf(bin1DIndex, nonSparseMarginalFreqs, sparseMarginal1DBinId);
    svtkm::cont::Algorithm::CopyIf(nonSparseMarginalFreqs, nonSparseMarginalFreqs, marginalFreqs);

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    svtkm::Id marginalVarIdx = numMarginalVariables - 1;
    for (svtkm::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const svtkm::Id nFieldBins = numBinsPortal.Get(i);
        svtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(
          sparseMarginal1DBinId, sparseMarginal1DBinId, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()

  // Execute the histogram marginalization WITHOUT CONDITION,
  // Please refer to the other Run() functions for the definition of input arguments.
  void Run(const std::vector<svtkm::cont::ArrayHandle<svtkm::Id>>& binId,
           svtkm::cont::ArrayHandle<svtkm::Id>& freqsIn,
           svtkm::cont::ArrayHandle<svtkm::Id>& numberOfBins,
           svtkm::cont::ArrayHandle<bool>& marginalVariables,
           std::vector<svtkm::cont::ArrayHandle<svtkm::Id>>& marginalBinId,
           svtkm::cont::ArrayHandle<svtkm::Id>& marginalFreqs)
  {
    //total variables
    svtkm::Id numOfVariable = static_cast<svtkm::Id>(binId.size());

    const svtkm::Id numberOfValues = freqsIn.GetNumberOfValues();
    svtkm::cont::ArrayHandleConstant<svtkm::Id> constant0Array(0, numberOfValues);
    svtkm::cont::ArrayHandle<svtkm::Id> bin1DIndex;
    svtkm::cont::ArrayCopy(constant0Array, bin1DIndex);

    svtkm::cont::ArrayHandle<svtkm::Id> freqs;
    svtkm::cont::ArrayCopy(freqsIn, freqs);
    svtkm::Id numMarginalVariables = 0; //count num of marginal variables
    const auto marginalPortal = marginalVariables.GetPortalConstControl();
    const auto numBinsPortal = numberOfBins.GetPortalConstControl();
    for (svtkm::Id i = 0; i < numOfVariable; i++)
    {
      if (marginalPortal.Get(i) == true)
      {
        // Worklet to calculate 1D index for marginal variables
        numMarginalVariables++;
        const svtkm::Id nFieldBins = numBinsPortal.Get(i);
        svtkm::worklet::histogram::To1DIndex binWorklet(nFieldBins);
        svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::To1DIndex> to1DIndexDispatcher(
          binWorklet);
        size_t vecIndex = static_cast<size_t>(i);
        to1DIndexDispatcher.Invoke(binId[vecIndex], bin1DIndex, bin1DIndex);
      }
    }

    // Sort the freq array for counting by key (1DIndex)
    svtkm::cont::Algorithm::SortByKey(bin1DIndex, freqs);

    // Add frequency within same 1d index bin
    svtkm::cont::Algorithm::ReduceByKey(bin1DIndex, freqs, bin1DIndex, marginalFreqs, svtkm::Add());

    //convert back to multi variate binId
    marginalBinId.resize(static_cast<size_t>(numMarginalVariables));
    svtkm::Id marginalVarIdx = numMarginalVariables - 1;
    for (svtkm::Id i = numOfVariable - 1; i >= 0; i--)
    {
      if (marginalPortal.Get(i) == true)
      {
        const svtkm::Id nFieldBins = numBinsPortal.Get(i);
        svtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
        svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::ConvertHistBinToND>
          convertHistBinToNDDispatcher(binWorklet);
        size_t vecIndex = static_cast<size_t>(marginalVarIdx);
        convertHistBinToNDDispatcher.Invoke(bin1DIndex, bin1DIndex, marginalBinId[vecIndex]);
        marginalVarIdx--;
      }
    }
  } //Run()
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_NDimsHistMarginalization_h
