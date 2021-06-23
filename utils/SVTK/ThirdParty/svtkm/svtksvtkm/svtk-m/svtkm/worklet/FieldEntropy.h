//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_FieldEntropy_h
#define svtk_m_worklet_FieldEntropy_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/FieldHistogram.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace worklet
{

//simple functor that returns basic statistics
class FieldEntropy
{
public:
  // For each bin, calculate its information content (log2)
  class SetBinInformationContent : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn freq, FieldOut informationContent);
    using ExecutionSignature = void(_1, _2);

    svtkm::Float64 FreqSum;

    SVTKM_CONT
    SetBinInformationContent(svtkm::Float64 _freqSum)
      : FreqSum(_freqSum)
    {
    }

    template <typename FreqType>
    SVTKM_EXEC void operator()(const FreqType& freq, svtkm::Float64& informationContent) const
    {
      svtkm::Float64 p = ((svtkm::Float64)freq) / FreqSum;
      if (p > 0)
        informationContent = -1 * p * svtkm::Log2(p);
      else
        informationContent = 0;
    }
  };


  // Execute the entropy computation filter given data(a field) and number of bins
  // Returns:
  // Entropy (log2) of the field of the data
  template <typename FieldType, typename Storage>
  svtkm::Float64 Run(svtkm::cont::ArrayHandle<FieldType, Storage> fieldArray, svtkm::Id numberOfBins)
  {
    ///// calculate histogram using FieldHistogram worklet /////
    svtkm::Range range;
    FieldType delta;
    svtkm::cont::ArrayHandle<svtkm::Id> binArray;
    svtkm::worklet::FieldHistogram histogram;
    histogram.Run(fieldArray, numberOfBins, range, delta, binArray);

    ///// calculate sum of frequency of the histogram /////
    svtkm::Id initFreqSumValue = 0;
    svtkm::Id freqSum = svtkm::cont::Algorithm::Reduce(binArray, initFreqSumValue, svtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    svtkm::cont::ArrayHandle<svtkm::Float64> informationContent;
    SetBinInformationContent binWorklet(static_cast<svtkm::Float64>(freqSum));
    svtkm::worklet::DispatcherMapField<SetBinInformationContent> setBinInformationContentDispatcher(
      binWorklet);
    setBinInformationContentDispatcher.Invoke(binArray, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    svtkm::Float64 initEntropyValue = 0;
    svtkm::Float64 entropy =
      svtkm::cont::Algorithm::Reduce(informationContent, initEntropyValue, svtkm::Sum());

    return entropy;
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_FieldEntropy_h
