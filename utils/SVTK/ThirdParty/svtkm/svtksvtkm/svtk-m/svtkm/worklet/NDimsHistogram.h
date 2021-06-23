//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_NDimsHistogram_h
#define svtk_m_worklet_NDimsHistogram_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>
#include <svtkm/worklet/histogram/ComputeNDHistogram.h>

#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace worklet
{

class NDimsHistogram
{
public:
  void SetNumOfDataPoints(svtkm::Id _numDataPoints)
  {
    NumDataPoints = _numDataPoints;

    // Initialize bin1DIndex array
    svtkm::cont::ArrayHandleConstant<svtkm::Id> constant0Array(0, NumDataPoints);
    svtkm::cont::ArrayCopy(constant0Array, Bin1DIndex);
  }

  // Add a field and the bin number for this field
  // Return: rangeOfRange is min max value of this array
  //         binDelta is delta of a bin
  template <typename HandleType>
  void AddField(const HandleType& fieldArray,
                svtkm::Id numberOfBins,
                svtkm::Range& rangeOfValues,
                svtkm::Float64& binDelta)
  {
    NumberOfBins.push_back(numberOfBins);

    if (fieldArray.GetNumberOfValues() != NumDataPoints)
    {
      throw svtkm::cont::ErrorBadValue("Array lengths does not match");
    }
    else
    {
      CastAndCall(
        fieldArray.ResetTypes(svtkm::TypeListScalarAll()),
        svtkm::worklet::histogram::ComputeBins(Bin1DIndex, numberOfBins, rangeOfValues, binDelta));
    }
  }

  // Execute N-Dim histogram worklet to get N-Dims histogram from input fields
  // Input arguments:
  //   binId: returned bin id of NDims-histogram, binId has n arrays, if length of fieldName is n
  //   freqs: returned frequency(count) array
  //     Note: the ND-histogram is returned in the fashion of sparse representation.
  //           (no zero frequency in freqs array)
  //           the length of all arrays in binId and freqs array must be the same
  //           if the length of fieldNames is n (compute a n-dimensional hisotgram)
  //           freqs[i] is the frequency of the bin with bin Ids{ binId[0][i], binId[1][i], ... binId[n-1][i] }
  void Run(std::vector<svtkm::cont::ArrayHandle<svtkm::Id>>& binId,
           svtkm::cont::ArrayHandle<svtkm::Id>& freqs)
  {
    binId.resize(NumberOfBins.size());

    // Sort the resulting bin(1D) array for counting
    svtkm::cont::Algorithm::Sort(Bin1DIndex);

    // Count frequency of each bin
    svtkm::cont::ArrayHandleConstant<svtkm::Id> constArray(1, NumDataPoints);
    svtkm::cont::Algorithm::ReduceByKey(Bin1DIndex, constArray, Bin1DIndex, freqs, svtkm::Add());

    //convert back to multi variate binId
    for (svtkm::Id i = static_cast<svtkm::Id>(NumberOfBins.size()) - 1; i >= 0; i--)
    {
      const svtkm::Id nFieldBins = NumberOfBins[static_cast<size_t>(i)];
      svtkm::worklet::histogram::ConvertHistBinToND binWorklet(nFieldBins);
      svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::ConvertHistBinToND>
        convertHistBinToNDDispatcher(binWorklet);
      size_t vectorId = static_cast<size_t>(i);
      convertHistBinToNDDispatcher.Invoke(Bin1DIndex, Bin1DIndex, binId[vectorId]);
    }
  }

private:
  std::vector<svtkm::Id> NumberOfBins;
  svtkm::cont::ArrayHandle<svtkm::Id> Bin1DIndex;
  svtkm::Id NumDataPoints;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_NDimsHistogram_h
