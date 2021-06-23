//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagKeys_h
#define svtk_m_cont_arg_TypeCheckTagKeys_h

#include <svtkm/cont/arg/TypeCheck.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// Check for a Keys object.
///
struct TypeCheckTagKeys
{
};

// A more specific specialization that actually checks for Keys types is
// implemented in svtkm/worklet/Keys.h. That class is not accessible from here
// due to SVTK-m package dependencies.
template <typename Type>
struct TypeCheck<TypeCheckTagKeys, Type>
{
  static constexpr bool value = false;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagKeys_h
