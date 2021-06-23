//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_FieldHistogram_h
#define svtk_m_worklet_FieldHistogram_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/Field.h>

namespace
{
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if defined(SVTKM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc
template <typename T>
T compute_delta(T fieldMinValue, T fieldMaxValue, svtkm::Id num)
{
  using VecType = svtkm::VecTraits<T>;
  const T fieldRange = fieldMaxValue - fieldMinValue;
  return fieldRange / static_cast<typename VecType::ComponentType>(num);
}
#if defined(SVTKM_GCC)
#pragma GCC diagnostic pop
#endif // gcc
}

namespace svtkm
{
namespace worklet
{

//simple functor that prints basic statistics
class FieldHistogram
{
public:
  // For each value set the bin it should be in
  template <typename FieldType>
  class SetHistogramBin : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value, FieldOut binIndex);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    svtkm::Id numberOfBins;
    FieldType minValue;
    FieldType delta;

    SVTKM_CONT
    SetHistogramBin(svtkm::Id numberOfBins0, FieldType minValue0, FieldType delta0)
      : numberOfBins(numberOfBins0)
      , minValue(minValue0)
      , delta(delta0)
    {
    }

    SVTKM_EXEC
    void operator()(const FieldType& value, svtkm::Id& binIndex) const
    {
      binIndex = static_cast<svtkm::Id>((value - minValue) / delta);
      if (binIndex < 0)
        binIndex = 0;
      else if (binIndex >= numberOfBins)
        binIndex = numberOfBins - 1;
    }
  };

  // Calculate the adjacent difference between values in ArrayHandle
  class AdjacentDifference : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inputIndex, WholeArrayIn counts, FieldOut outputCount);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template <typename WholeArrayType>
    SVTKM_EXEC void operator()(const svtkm::Id& index,
                              const WholeArrayType& counts,
                              svtkm::Id& difference) const
    {
      if (index == 0)
        difference = counts.Get(index);
      else
        difference = counts.Get(index) - counts.Get(index - 1);
    }
  };

  // Execute the histogram binning filter given data and number of bins
  // Returns:
  // min value of the bins
  // delta/range of each bin
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(svtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           svtkm::Id numberOfBins,
           svtkm::Range& rangeOfValues,
           FieldType& binDelta,
           svtkm::cont::ArrayHandle<svtkm::Id>& binArray)
  {
    const svtkm::Vec<FieldType, 2> initValue{ svtkm::cont::ArrayGetValue(0, fieldArray) };

    svtkm::Vec<FieldType, 2> result =
      svtkm::cont::Algorithm::Reduce(fieldArray, initValue, svtkm::MinAndMax<FieldType>());

    this->Run(fieldArray, numberOfBins, result[0], result[1], binDelta, binArray);

    //update the users data
    rangeOfValues = svtkm::Range(result[0], result[1]);
  }

  // Execute the histogram binning filter given data and number of bins, min,
  // max values.
  // Returns:
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(svtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           svtkm::Id numberOfBins,
           FieldType fieldMinValue,
           FieldType fieldMaxValue,
           FieldType& binDelta,
           svtkm::cont::ArrayHandle<svtkm::Id>& binArray)
  {
    const svtkm::Id numberOfValues = fieldArray.GetNumberOfValues();

    const FieldType fieldDelta = compute_delta(fieldMinValue, fieldMaxValue, numberOfBins);

    // Worklet fills in the bin belonging to each value
    svtkm::cont::ArrayHandle<svtkm::Id> binIndex;
    binIndex.Allocate(numberOfValues);

    // Worklet to set the bin number for each data value
    SetHistogramBin<FieldType> binWorklet(numberOfBins, fieldMinValue, fieldDelta);
    svtkm::worklet::DispatcherMapField<SetHistogramBin<FieldType>> setHistogramBinDispatcher(
      binWorklet);
    setHistogramBinDispatcher.Invoke(fieldArray, binIndex);

    // Sort the resulting bin array for counting
    svtkm::cont::Algorithm::Sort(binIndex);

    // Get the upper bound of each bin number
    svtkm::cont::ArrayHandle<svtkm::Id> totalCount;
    svtkm::cont::ArrayHandleCounting<svtkm::Id> binCounter(0, 1, numberOfBins);
    svtkm::cont::Algorithm::UpperBounds(binIndex, binCounter, totalCount);

    // Difference between adjacent items is the bin count
    svtkm::worklet::DispatcherMapField<AdjacentDifference> dispatcher;
    dispatcher.Invoke(binCounter, totalCount, binArray);

    //update the users data
    binDelta = fieldDelta;
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_FieldHistogram_h
