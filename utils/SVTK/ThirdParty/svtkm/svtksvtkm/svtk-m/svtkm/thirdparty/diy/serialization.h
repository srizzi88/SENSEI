//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_thirdparty_diy_serialization_h
#define svtk_m_thirdparty_diy_serialization_h

#include <svtkm/thirdparty/diy/Configure.h>

#if SVTKM_USE_EXTERNAL_DIY
#define SVTKM_DIY_INCLUDE(header) <diy/header>
#else
#define SVTKM_DIY_INCLUDE(header) <svtkmdiy/include/svtkmdiy/header>
#define diy svtkmdiy // mangle namespace diy (see below comments)
#endif

#if defined(SVTKM_CLANG) || defined(SVTKM_GCC)
#pragma GCC visibility push(default)
#endif

// clang-format off
SVTKM_THIRDPARTY_PRE_INCLUDE
#include SVTKM_DIY_INCLUDE(serialization.hpp)
#undef SVTKM_DIY_INCLUDE
SVTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#if defined(SVTKM_CLANG) || defined(SVTKM_GCC)
#pragma GCC visibility pop
#endif

// When using an external DIY
// We need to alias the diy namespace to
// svtkmdiy so that SVTK-m uses it properly
#if SVTKM_USE_EXTERNAL_DIY
namespace svtkmdiy = ::diy;

#else
// The aliasing approach fails for when we
// want to us an internal version since
// the diy namespace already points to the
// external version. Instead we use macro
// replacement to make sure all diy classes
// are placed in svtkmdiy placed
#undef diy // mangle namespace diy

#endif

#endif
