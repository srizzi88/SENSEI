//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ScatterIdentity_h
#define svtk_m_worklet_ScatterIdentity_h

#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/worklet/internal/ScatterBase.h>

namespace svtkm
{
namespace worklet
{

/// \brief A scatter that maps input directly to output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterIdentity establishes a 1 to
/// 1 mapping from input to output (and vice versa). That is, every input
/// element generates one output element associated with it. This is the
/// default for basic maps.
///
struct ScatterIdentity : internal::ScatterBase
{
  using OutputToInputMapType = svtkm::cont::ArrayHandleIndex;
  SVTKM_CONT
  OutputToInputMapType GetOutputToInputMap(svtkm::Id inputRange) const
  {
    return OutputToInputMapType(inputRange);
  }
  SVTKM_CONT
  OutputToInputMapType GetOutputToInputMap(svtkm::Id3 inputRange) const
  {
    return this->GetOutputToInputMap(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using VisitArrayType = svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>;
  SVTKM_CONT
  VisitArrayType GetVisitArray(svtkm::Id inputRange) const { return VisitArrayType(0, inputRange); }
  SVTKM_CONT
  VisitArrayType GetVisitArray(svtkm::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  template <typename RangeType>
  SVTKM_CONT RangeType GetOutputRange(RangeType inputRange) const
  {
    return inputRange;
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_ScatterIdentity_h
