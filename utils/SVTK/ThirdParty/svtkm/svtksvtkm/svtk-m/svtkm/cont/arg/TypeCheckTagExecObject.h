//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagExecObject_h
#define svtk_m_cont_arg_TypeCheckTagExecObject_h

#include <svtkm/internal/ExportMacros.h>

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/cont/ExecutionObjectBase.h>

#include <type_traits>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// The ExecObject type check passes for any object that inherits from \c
/// ExecutionObjectBase. This is supposed to signify that the object can be
/// used in the execution environment although there is no way to verify that.
///
struct TypeCheckTagExecObject
{
};

template <typename Type>
struct TypeCheck<TypeCheckTagExecObject, Type>
{
  static constexpr bool value = svtkm::cont::internal::IsExecutionObjectBase<Type>::value;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagExecObject_h
