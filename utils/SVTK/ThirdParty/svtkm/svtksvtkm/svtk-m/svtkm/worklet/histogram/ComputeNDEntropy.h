//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_ComputeNDEntropy_h
#define svtk_m_worklet_ComputeNDEntropy_h

#include <svtkm/worklet/DispatcherMapField.h>

namespace svtkm
{
namespace worklet
{
namespace histogram
{
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
}
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_ComputeNDEntropy_h
