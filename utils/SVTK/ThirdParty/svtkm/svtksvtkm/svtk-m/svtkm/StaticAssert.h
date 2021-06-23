//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_StaticAssert_h
#define svtk_m_StaticAssert_h


#include <type_traits>

#define SVTKM_STATIC_ASSERT(condition)                                                              \
  static_assert((condition), "Failed static assert: " #condition)
#define SVTKM_STATIC_ASSERT_MSG(condition, message) static_assert((condition), message)

namespace svtkm
{

template <bool noError>
struct ReadTheSourceCodeHereForHelpOnThisError;

template <>
struct ReadTheSourceCodeHereForHelpOnThisError<true> : std::true_type
{
};

} // namespace svtkm

#define SVTKM_READ_THE_SOURCE_CODE_FOR_HELP(noError)                                                \
  SVTKM_STATIC_ASSERT(svtkm::ReadTheSourceCodeHereForHelpOnThisError<noError>::value)

#endif //svtk_m_StaticAssert_h
