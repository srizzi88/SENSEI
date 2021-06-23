//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Unreachable_h
#define svtk_m_Unreachable_h

/// SVTKM_UNREACHABLE is similar to SVTK_ASSUME, with the significant difference
/// that it is not conditional. Control should never reach a path containing
/// a SVTKM_UNREACHABLE statement under any circumstances.
///
/// If assertions are enabled (e.g. neither NDEBUG nor SVTKM_NO_ASSERT is
/// defined), the following steps are taken:
/// 1. Print an error message containing the macro argument and location of the
///    SVTKM_UNREACHABLE call.
/// 2. Abort the kernel (if CUDA) or process.
///
/// This allows bad code paths to be identified during development and
/// debugging.
///
/// If assertions are disabled and the compiler has some sort of 'unreachable'
/// intrinsic used to provide optimization hints, the intrinsic is used to
/// notify the compiler that this is a dead code path.
///
#define SVTKM_UNREACHABLE(msg)                                                                      \
  SVTKM_SWALLOW_SEMICOLON_PRE_BLOCK                                                                 \
  {                                                                                                \
    SVTKM_UNREACHABLE_IMPL();                                                                       \
    SVTKM_UNREACHABLE_PRINT(msg);                                                                   \
    SVTKM_UNREACHABLE_ABORT();                                                                      \
  }                                                                                                \
  SVTKM_SWALLOW_SEMICOLON_POST_BLOCK

// SVTKM_UNREACHABLE_IMPL is compiler-specific:
#if defined(SVTKM_CUDA_DEVICE_PASS)

#define SVTKM_UNREACHABLE_IMPL() (void)0 /* no-op, no known intrinsic */

#if defined(NDEBUG) || defined(SVTKM_NO_ASSERT)

#define SVTKM_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define SVTKM_UNREACHABLE_ABORT() (void)0    /* no-op */

#else // NDEBUG || SVTKM_NO_ASSERT

#define SVTKM_UNREACHABLE_PRINT(msg)                                                                \
  printf("Unreachable location reached: %s\nLocation: %s:%d\n", msg, __FILE__, __LINE__)
#define SVTKM_UNREACHABLE_ABORT()                                                                   \
  asm("trap;") /* Triggers kernel exit with CUDA error 73: Illegal inst */

#endif // NDEBUG || SVTKM_NO_ASSERT

#else // !CUDA


#if defined(NDEBUG) || defined(SVTKM_NO_ASSERT)

#define SVTKM_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define SVTKM_UNREACHABLE_ABORT() (void)0    /* no-op */

#if defined(SVTKM_MSVC)
#define SVTKM_UNREACHABLE_IMPL() __assume(false)
#elif defined(SVTKM_ICC) && !defined(__GNUC__)
#define SVTKM_UNREACHABLE_IMPL() __assume(false)
#elif (defined(SVTKM_GCC) || defined(SVTKM_ICC)) &&                                                  \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
// Added in 4.5.0:
#define SVTKM_UNREACHABLE_IMPL() __builtin_unreachable()
#elif defined(SVTKM_CLANG)
#define SVTKM_UNREACHABLE_IMPL() __builtin_unreachable()
#else
#define SVTKM_UNREACHABLE_IMPL() (void)0 /* no-op */
#endif

#else // NDEBUG || SVTKM_NO_ASSERT

#define SVTKM_UNREACHABLE_IMPL() (void)0
#define SVTKM_UNREACHABLE_PRINT(msg)                                                                \
  std::cerr << "Unreachable location reached: " << msg << "\n"                                     \
            << "Location: " << __FILE__ << ":" << __LINE__ << "\n"
#define SVTKM_UNREACHABLE_ABORT() abort()

#endif // NDEBUG && !SVTKM_NO_ASSERT

#endif // !CUDA

#endif //svtk_m_Unreachable_h
