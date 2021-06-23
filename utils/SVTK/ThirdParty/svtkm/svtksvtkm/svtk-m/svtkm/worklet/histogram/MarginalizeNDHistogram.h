//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_MarginalizeNDHistogram_h
#define svtk_m_worklet_MarginalizeNDHistogram_h

#include <svtkm/worklet/DispatcherMapField.h>

namespace svtkm
{
namespace worklet
{
namespace histogram
{
// Set freq of the entity, which does not meet the condition, to 0
template <class BinaryCompare>
class ConditionalFreq : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  ConditionalFreq(BinaryCompare _bop)
    : bop(_bop)
  {
  }

  SVTKM_CONT
  void setVar(svtkm::Id _var) { var = _var; }

  BinaryCompare bop;
  svtkm::Id var;

  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  SVTKM_EXEC
  void operator()(const svtkm::Id& binIdIn, const svtkm::Id& freqIn, svtkm::Id& freqOut) const
  {
    if (bop(var, binIdIn))
      freqOut = freqIn;
    else
      freqOut = 0;
  }
};

// Convert multiple indices to 1D index
class To1DIndex : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn bin, FieldIn binIndexIn, FieldOut binIndexOut);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  svtkm::Id numberOfBins;

  SVTKM_CONT
  To1DIndex(svtkm::Id numberOfBins0)
    : numberOfBins(numberOfBins0)
  {
  }

  SVTKM_EXEC
  void operator()(const svtkm::Id& bin, const svtkm::Id& binIndexIn, svtkm::Id& binIndexOut) const
  {
    binIndexOut = binIndexIn * numberOfBins + bin;
  }
};
}
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_MarginalizeNDHistogram_h
