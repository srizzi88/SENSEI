//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ScatterPermutation_h
#define svtk_m_worklet_ScatterPermutation_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/worklet/internal/ScatterBase.h>

namespace svtkm
{
namespace worklet
{

/// \brief A scatter that maps input to output based on a permutation array.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterPermutation is similar to
/// \c ScatterCounting but can have lesser memory usage for some cases.
/// The constructor takes an array of ids, where each entry maps the
/// corresponding output to an input. The ids can be in any order and there
/// can be duplicates. Note that even with duplicates the VistIndex is always 0.
///
template <typename PermutationStorage = SVTKM_DEFAULT_STORAGE_TAG>
class ScatterPermutation : public internal::ScatterBase
{
private:
  using PermutationArrayHandle = svtkm::cont::ArrayHandle<svtkm::Id, PermutationStorage>;

public:
  using OutputToInputMapType = PermutationArrayHandle;
  using VisitArrayType = svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>;

  ScatterPermutation(const PermutationArrayHandle& permutation)
    : Permutation(permutation)
  {
  }

  SVTKM_CONT
  template <typename RangeType>
  svtkm::Id GetOutputRange(RangeType) const
  {
    return this->Permutation.GetNumberOfValues();
  }

  template <typename RangeType>
  SVTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType) const
  {
    return this->Permutation;
  }

  SVTKM_CONT OutputToInputMapType GetOutputToInputMap() const { return this->Permutation; }

  SVTKM_CONT
  VisitArrayType GetVisitArray(svtkm::Id inputRange) const { return VisitArrayType(0, inputRange); }

  SVTKM_CONT
  VisitArrayType GetVisitArray(svtkm::Id3 inputRange) const
  {
    return this->GetVisitArray(inputRange[0] * inputRange[1] * inputRange[2]);
  }

private:
  PermutationArrayHandle Permutation;
};
}
} // svtkm::worklet

#endif // svtk_m_worklet_ScatterPermutation_h
