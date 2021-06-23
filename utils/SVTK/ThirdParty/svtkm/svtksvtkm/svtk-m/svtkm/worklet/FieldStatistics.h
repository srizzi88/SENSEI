//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_FieldStatistics_h
#define svtk_m_worklet_FieldStatistics_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/Field.h>

#include <stdio.h>

namespace svtkm
{
namespace worklet
{

//simple functor that prints basic statistics
template <typename FieldType>
class FieldStatistics
{
public:
  // For moments readability
  static constexpr svtkm::Id FIRST = 0;
  static constexpr svtkm::Id SECOND = 1;
  static constexpr svtkm::Id THIRD = 2;
  static constexpr svtkm::Id FOURTH = 3;
  static constexpr svtkm::Id NUM_POWERS = 4;

  struct StatInfo
  {
    FieldType minimum;
    FieldType maximum;
    FieldType median;
    FieldType mean;
    FieldType variance;
    FieldType stddev;
    FieldType skewness;
    FieldType kurtosis;
    FieldType rawMoment[4];
    FieldType centralMoment[4];
  };

  class CalculatePowers : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value,
                                  FieldOut pow1Array,
                                  FieldOut pow2Array,
                                  FieldOut pow3Array,
                                  FieldOut pow4Array);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);
    using InputDomain = _1;

    svtkm::Id numPowers;

    SVTKM_CONT
    CalculatePowers(svtkm::Id num)
      : numPowers(num)
    {
    }

    SVTKM_EXEC
    void operator()(const FieldType& value,
                    FieldType& pow1,
                    FieldType& pow2,
                    FieldType& pow3,
                    FieldType& pow4) const
    {
      pow1 = value;
      pow2 = pow1 * value;
      pow3 = pow2 * value;
      pow4 = pow3 * value;
    }
  };

  class SubtractConst : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value, FieldOut diff);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    FieldType constant;

    SVTKM_CONT
    SubtractConst(const FieldType& constant0)
      : constant(constant0)
    {
    }

    SVTKM_EXEC
    FieldType operator()(const FieldType& value) const { return (value - constant); }
  };

  template <typename Storage>
  void Run(svtkm::cont::ArrayHandle<FieldType, Storage> fieldArray, StatInfo& statinfo)
  {
    using DeviceAlgorithms = svtkm::cont::Algorithm;

    // Copy original data to array for sorting
    svtkm::cont::ArrayHandle<FieldType> tempArray;
    DeviceAlgorithms::Copy(fieldArray, tempArray);
    DeviceAlgorithms::Sort(tempArray);

    svtkm::Id dataSize = tempArray.GetNumberOfValues();
    FieldType numValues = static_cast<FieldType>(dataSize);
    const auto firstAndMedian = svtkm::cont::ArrayGetValues({ 0, dataSize / 2 }, tempArray);

    // Median
    statinfo.median = firstAndMedian[1];

    // Minimum and maximum
    const svtkm::Vec<FieldType, 2> initValue(firstAndMedian[0]);
    svtkm::Vec<FieldType, 2> result =
      DeviceAlgorithms::Reduce(fieldArray, initValue, svtkm::MinAndMax<FieldType>());
    statinfo.minimum = result[0];
    statinfo.maximum = result[1];

    // Mean
    FieldType sum = DeviceAlgorithms::ScanInclusive(fieldArray, tempArray);
    statinfo.mean = sum / numValues;
    statinfo.rawMoment[FIRST] = sum / numValues;

    // Create the power sum vector for each value
    svtkm::cont::ArrayHandle<FieldType> pow1Array, pow2Array, pow3Array, pow4Array;
    pow1Array.Allocate(dataSize);
    pow2Array.Allocate(dataSize);
    pow3Array.Allocate(dataSize);
    pow4Array.Allocate(dataSize);

    // Raw moments via Worklet
    svtkm::worklet::DispatcherMapField<CalculatePowers> calculatePowersDispatcher(
      CalculatePowers(4));
    calculatePowersDispatcher.Invoke(fieldArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.rawMoment[FIRST] = DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.rawMoment[SECOND] = DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.rawMoment[THIRD] = DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.rawMoment[FOURTH] = DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Subtract the mean from every value and leave in tempArray
    svtkm::worklet::DispatcherMapField<SubtractConst> subtractConstDispatcher(
      SubtractConst(statinfo.mean));
    subtractConstDispatcher.Invoke(fieldArray, tempArray);

    // Calculate sums of powers on the (value - mean) array
    calculatePowersDispatcher.Invoke(tempArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.centralMoment[FIRST] =
      DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.centralMoment[SECOND] =
      DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.centralMoment[THIRD] =
      DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.centralMoment[FOURTH] =
      DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Statistics from the moments
    statinfo.variance = statinfo.centralMoment[SECOND];
    statinfo.stddev = Sqrt(statinfo.variance);
    statinfo.skewness =
      statinfo.centralMoment[THIRD] / Pow(statinfo.stddev, static_cast<FieldType>(3.0));
    statinfo.kurtosis =
      statinfo.centralMoment[FOURTH] / Pow(statinfo.stddev, static_cast<FieldType>(4.0));
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_FieldStatistics_h
