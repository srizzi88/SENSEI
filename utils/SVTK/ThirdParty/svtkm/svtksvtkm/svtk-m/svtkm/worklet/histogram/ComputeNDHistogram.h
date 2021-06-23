//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_ComputeNDHistogram_h
#define svtk_m_worklet_ComputeNDHistogram_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>

#include <svtkm/worklet/DispatcherMapField.h>

namespace svtkm
{
namespace worklet
{
namespace histogram
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

// For each value set the bin it should be in
template <typename FieldType>
class SetHistogramBin : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn value, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  svtkm::Id numberOfBins;
  svtkm::Float64 minValue;
  svtkm::Float64 delta;

  SVTKM_CONT
  SetHistogramBin(svtkm::Id numberOfBins0, svtkm::Float64 minValue0, svtkm::Float64 delta0)
    : numberOfBins(numberOfBins0)
    , minValue(minValue0)
    , delta(delta0)
  {
  }

  SVTKM_EXEC
  void operator()(const FieldType& value, const svtkm::Id& binIndexIn, svtkm::Id& binIndexOut) const
  {
    const svtkm::Float64 fvalue = static_cast<svtkm::Float64>(value);
    svtkm::Id localBinIdx = static_cast<svtkm::Id>((fvalue - minValue) / delta);
    if (localBinIdx < 0)
      localBinIdx = 0;
    else if (localBinIdx >= numberOfBins)
      localBinIdx = numberOfBins - 1;

    binIndexOut = binIndexIn * numberOfBins + localBinIdx;
  }
};

class ComputeBins
{
public:
  SVTKM_CONT
  ComputeBins(svtkm::cont::ArrayHandle<svtkm::Id>& _bin1DIdx,
              svtkm::Id& _numOfBins,
              svtkm::Range& _minMax,
              svtkm::Float64& _binDelta)
    : Bin1DIdx(_bin1DIdx)
    , NumOfBins(_numOfBins)
    , MinMax(_minMax)
    , BinDelta(_binDelta)
  {
  }

  template <typename T, typename Storage>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    const svtkm::Vec<T, 2> initValue(svtkm::cont::ArrayGetValue(0, field));
    svtkm::Vec<T, 2> minMax = svtkm::cont::Algorithm::Reduce(field, initValue, svtkm::MinAndMax<T>());
    MinMax.Min = static_cast<svtkm::Float64>(minMax[0]);
    MinMax.Max = static_cast<svtkm::Float64>(minMax[1]);
    BinDelta = compute_delta(MinMax.Min, MinMax.Max, NumOfBins);

    SetHistogramBin<T> binWorklet(NumOfBins, MinMax.Min, BinDelta);
    svtkm::worklet::DispatcherMapField<svtkm::worklet::histogram::SetHistogramBin<T>>
      setHistogramBinDispatcher(binWorklet);
    setHistogramBinDispatcher.Invoke(field, Bin1DIdx, Bin1DIdx);
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id>& Bin1DIdx;
  svtkm::Id& NumOfBins;
  svtkm::Range& MinMax;
  svtkm::Float64& BinDelta;
};

// Convert N-dims bin index into 1D index
class ConvertHistBinToND : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin1DIndexIn,
                                FieldOut bin1DIndexOut,
                                FieldOut oneVariableIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  svtkm::Id numberOfBins;

  SVTKM_CONT
  ConvertHistBinToND(svtkm::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  SVTKM_EXEC
  void operator()(const svtkm::Id& bin1DIndexIn,
                  svtkm::Id& bin1DIndexOut,
                  svtkm::Id& oneVariableIndexOut) const
  {
    oneVariableIndexOut = bin1DIndexIn % numberOfBins;
    bin1DIndexOut = (bin1DIndexIn - oneVariableIndexOut) / numberOfBins;
  }
};
}
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_ComputeNDHistogram_h
