//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagArray_h
#define svtk_m_cont_arg_TypeCheckTagArray_h

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/List.h>

#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// The Array type check passes for any object that behaves like an \c
/// ArrayHandle class and can be passed to the ArrayIn and ArrayOut transports.
///
struct TypeCheckTagArray
{
};

template <typename ArrayType>
struct TypeCheck<TypeCheckTagArray, ArrayType>
{
  static constexpr bool value = svtkm::cont::internal::ArrayHandleCheck<ArrayType>::type::value;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagArray_h
