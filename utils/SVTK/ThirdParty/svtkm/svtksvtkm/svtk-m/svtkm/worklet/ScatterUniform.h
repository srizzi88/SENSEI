//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ScatterUniform_h
#define svtk_m_worklet_ScatterUniform_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/worklet/internal/ScatterBase.h>

namespace svtkm
{
namespace worklet
{

namespace detail
{

template <svtkm::IdComponent Modulus>
struct FunctorModulus
{
  SVTKM_EXEC_CONT
  svtkm::IdComponent operator()(svtkm::Id index) const
  {
    return static_cast<svtkm::IdComponent>(index % Modulus);
  }
};

template <svtkm::IdComponent Divisor>
struct FunctorDiv
{
  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id index) const { return index / Divisor; }
};
}

/// \brief A scatter that maps input to some constant numbers of output.
///
/// The \c Scatter classes are responsible for defining how much output is
/// generated based on some sized input. \c ScatterUniform establishes a 1 to N
/// mapping from input to output. That is, every input element generates N
/// elements associated with it where N is the same for every input. The output
/// elements are grouped by the input associated.
///
template <svtkm::IdComponent NumOutputsPerInput>
struct ScatterUniform : internal::ScatterBase
{
  SVTKM_CONT ScatterUniform() = default;

  SVTKM_CONT
  svtkm::Id GetOutputRange(svtkm::Id inputRange) const { return inputRange * NumOutputsPerInput; }
  SVTKM_CONT
  svtkm::Id GetOutputRange(svtkm::Id3 inputRange) const
  {
    return this->GetOutputRange(inputRange[0] * inputRange[1] * inputRange[2]);
  }

  using OutputToInputMapType =
    svtkm::cont::ArrayHandleImplicit<detail::FunctorDiv<NumOutputsPerInput>>;
  template <typename RangeType>
  SVTKM_CONT OutputToInputMapType GetOutputToInputMap(RangeType inputRange) const
  {
    return OutputToInputMapType(detail::FunctorDiv<NumOutputsPerInput>(),
                                this->GetOutputRange(inputRange));
  }

  using VisitArrayType =
    svtkm::cont::ArrayHandleImplicit<detail::FunctorModulus<NumOutputsPerInput>>;
  template <typename RangeType>
  SVTKM_CONT VisitArrayType GetVisitArray(RangeType inputRange) const
  {
    return VisitArrayType(detail::FunctorModulus<NumOutputsPerInput>(),
                          this->GetOutputRange(inputRange));
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_ScatterUniform_h
