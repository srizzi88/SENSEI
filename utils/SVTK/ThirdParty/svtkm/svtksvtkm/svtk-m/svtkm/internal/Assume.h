//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_Assume_h
#define svtk_m_internal_Assume_h

#include <svtkm/Assert.h>

// Description:
// SVTKM_ASSUME instructs the compiler that a certain non-obvious condition will
// *always* be true. Beware that if cond is false at runtime, the results are
// unpredictable (and likely catastrophic). A runtime assertion is added so
// that debugging builds may easily catch violations of the condition.
//
// A useful application of this macro is when a method is passed in a
// svtkm::Vec that is uninitialized and conditional fills the svtkm::Vec
// based on other runtime information such as cell type. This allows you to
// assert that only valid cell types will be used, producing more efficient
// code.
//
#define SVTKM_ASSUME(cond)                                                                          \
  SVTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    const bool c = cond;                                                                           \
    SVTKM_ASSERT("Bad assumption in SVTKM_ASSUME: " #cond&& c);                                      \
    SVTKM_ASSUME_IMPL(c);                                                                           \
    (void)c; /* Prevents unused var warnings */                                                    \
  }                                                                                                \
  SVTKM_SWALLOW_SEMICOLON_POST_BLOCK

// SVTKM_ASSUME_IMPL is compiler-specific:
#if defined(SVTKM_CUDA_DEVICE_PASS)
//For all versions of CUDA this is a no-op while we look
//for a CUDA asm snippet that replicates this kind of behavior
#define SVTKM_ASSUME_IMPL(cond) (void)0 /* no-op */
#else

#if defined(SVTKM_MSVC)
#define SVTKM_ASSUME_IMPL(cond) __assume(cond)
#elif defined(SVTKM_ICC) && !defined(__GNUC__)
#define SVTKM_ASSUME_IMPL(cond) __assume(cond)
#elif (defined(SVTKM_GCC) || defined(SVTKM_ICC)) &&                                                  \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
// Added in 4.5.0:
#define SVTKM_ASSUME_IMPL(cond)                                                                     \
  if (!(cond))                                                                                     \
  __builtin_unreachable()
#elif defined(SVTKM_CLANG)
#define SVTKM_ASSUME_IMPL(cond)                                                                     \
  if (!(cond))                                                                                     \
  __builtin_unreachable()
#else
#define SVTKM_ASSUME_IMPL(cond) (void)0 /* no-op */
#endif

#endif

#endif // svtk_m_internal_Assume_h
