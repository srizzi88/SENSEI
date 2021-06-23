//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_MaskNone_h
#define svtk_m_worklet_MaskNone_h

#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/worklet/internal/MaskBase.h>

namespace svtkm
{
namespace worklet
{

/// \brief Default mask object that does not suppress anything.
///
/// \c MaskNone is a worklet mask object that does not suppress any items in the output
/// domain. This is the default mask object so that the worklet is run for every possible
/// output element.
///
struct MaskNone : public internal::MaskBase
{
  template <typename RangeType>
  SVTKM_CONT RangeType GetThreadRange(RangeType outputRange) const
  {
    return outputRange;
  }

  using ThreadToOutputMapType = svtkm::cont::ArrayHandleIndex;

  SVTKM_CONT ThreadToOutputMapType GetThreadToOutputMap(svtkm::Id outputRange) const
  {
    return svtkm::cont::ArrayHandleIndex(outputRange);
  }

  SVTKM_CONT ThreadToOutputMapType GetThreadToOutputMap(const svtkm::Id3& outputRange) const
  {
    return this->GetThreadToOutputMap(outputRange[0] * outputRange[1] * outputRange[2]);
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_MaskNone_h
