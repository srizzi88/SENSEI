//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_NDimsEntropy_h
#define svtk_m_worklet_NDimsEntropy_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/NDimsHistogram.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/histogram/ComputeNDEntropy.h>

#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace worklet
{

class NDimsEntropy
{
public:
  void SetNumOfDataPoints(svtkm::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;
    NdHistogram.SetNumOfDataPoints(_numDataPoints);
  }

  // Add a field and the bin for this field
  // Return: rangeOfRange is min max value of this array
  //         binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray, svtkm::Id numberOfBins)
  {
    svtkm::Range range;
    svtkm::Float64 delta;

    NdHistogram.AddField(fieldArray, numberOfBins, range, delta);
  }

  // Execute the entropy computation filter given
  // fields and number of bins of each fields
  // Returns:
  // Entropy (log2) of the field of the data
  svtkm::Float64 Run()
  {
    std::vector<svtkm::cont::ArrayHandle<svtkm::Id>> binIds;
    svtkm::cont::ArrayHandle<svtkm::Id> freqs;
    NdHistogram.Run(binIds, freqs);

    ///// calculate sum of frequency of the histogram /////
    svtkm::Id initFreqSumValue = 0;
    svtkm::Id freqSum = svtkm::cont::Algorithm::Reduce(freqs, initFreqSumValue, svtkm::Sum());

    ///// calculate information content of each bin using self-define worklet /////
    svtkm::cont::ArrayHandle<svtkm::Float64> informationContent;
    svtkm::worklet::histogram::SetBinInformationContent binWorklet(
      static_cast<svtkm::Float64>(freqSum));
    svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::SetBinInformationContent>
      setBinInformationContentDispatcher(binWorklet);
    setBinInformationContentDispatcher.Invoke(freqs, informationContent);

    ///// calculate entropy by summing up information conetent of all bins /////
    svtkm::Float64 initEntropyValue = 0;
    svtkm::Float64 entropy =
      svtkm::cont::Algorithm::Reduce(informationContent, initEntropyValue, svtkm::Sum());

    return entropy;
  }

private:
  svtkm::worklet::NDimsHistogram NdHistogram;
  svtkm::Id NumDataPoints;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_NDimsEntropy_h
