//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_Deprecated_h
#define svtk_m_Deprecated_h

#include <svtkm/StaticAssert.h>
#include <svtkm/Types.h>

#define SVTK_M_DEPRECATED_MAKE_MESSAGE(...)                                                         \
  SVTKM_EXPAND(SVTK_M_DEPRECATED_MAKE_MESSAGE_IMPL(__VA_ARGS__, "", svtkm::internal::NullType{}))
#define SVTK_M_DEPRECATED_MAKE_MESSAGE_IMPL(version, message, ...)                                  \
  message " Deprecated in version " #version "."

/// \def SVTKM_DEPRECATED(version, message)
///
/// Classes and methods are marked deprecated using the `SVTKM_DEPRECATED`
/// macro. The first argument of `SVTKM_DEPRECATED` should be set to the first
/// version in which the feature is deprecated. For example, if the last
/// released version of SVTK-m was 1.5, and on the master branch a developer
/// wants to deprecate a class foo, then the `SVTKM_DEPRECATED` release version
/// should be given as 1.6, which will be the next minor release of SVTK-m. The
/// second argument of `SVTKM_DEPRECATED`, which is optional but highly
/// encouraged, is a short message that should clue developers on how to update
/// their code to the new changes. For example, it could point to the
/// replacement class or method for the changed feature.
///

/// \def SVTKM_DEPRECATED_SUPPRESS_BEGIN
///
/// Begins a region of code in which warnings about using deprecated code are ignored.
/// Such suppression is usually helpful when implementing other deprecated features.
/// (You would think if one deprecated method used another deprecated method this
/// would not be a warning, but it is.)
///
/// Any use of `SVTKM_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `SVTKM_DEPRECATED_SUPPRESS_END`, which will re-enable warnings in subsequent code.
///
/// Do not use a semicolon after this macro.
///

/// \def SVTKM_DEPRECATED_SUPPRESS_END
///
/// Ends a region of code in which warnings about using deprecated code are ignored.
/// Any use of `SVTKM_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `SVTKM_DEPRECATED_SUPPRESS_END`.
///
/// Do not use a semicolon after this macro.
///

// Determine whether the [[deprecated]] attribute is supported. Note that we are not
// using other older compiler features such as __attribute__((__deprecated__)) because
// they do not all support all [[deprecated]] uses (such as uses in enums). If
// [[deprecated]] is supported, then SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED will get defined.
#ifndef SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#if __cplusplus >= 201402L

// C++14 and better supports [[deprecated]]
#define SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#elif defined(SVTKM_GCC)

// GCC has supported [[deprecated]] since version 5.0, but using it on enum was not
// supported until 6.0. So we have to make a special case to only use it for high
// enough revisions.
#if __GNUC__ >= 6
#define SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // Too old GCC

#elif defined(__has_cpp_attribute)

#if __has_cpp_attribute(deprecated)
// Compiler not fully C++14 compliant, but it reports to support [[deprecated]]
#define SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // __has_cpp_attribute(deprecated)

#elif defined(SVTKM_MSVC) && _MSC_VER >= 1900

#define SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED

#endif // no known compiler support for [[deprecated]]

#endif // SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED check

// Determine how to turn deprecated warnings on and off, generally with pragmas. If
// deprecated warnings can be turned off and on, then SVTK_M_DEPRECATED_SUPPRESS_SUPPORTED
// is defined and the pair SVTKM_DEPRECATED_SUPPRESS_BEGIN and SVTKM_DEPRECATED_SUPRESS_END
// are defined to the pragmas to disable and restore these warnings. If this support
// cannot be determined, SVTK_M_DEPRECATED_SUPPRESS_SUPPORTED is _not_ define whereas
// SVTKM_DEPRECATED_SUPPRESS_BEGIN and SVTKM_DEPRECATED_SUPPRESS_END are defined to be
// empty.
#ifndef SVTKM_DEPRECATED_SUPPRESS_SUPPORTED

#if defined(SVTKM_GCC) || defined(SVTKM_CLANG)

#define SVTKM_DEPRECATED_SUPPRESS_SUPPORTED
#define SVTKM_DEPRECATED_SUPPRESS_BEGIN                                                             \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define SVTKM_DEPRECATED_SUPPRESS_END _Pragma("GCC diagnostic pop")

#elif defined(SVTKM_MSVC)

#define SVTKM_DEPRECATED_SUPPRESS_SUPPORTED
#define SVTKM_DEPRECATED_SUPPRESS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define SVTKM_DEPRECATED_SUPPRESS_END __pragma(warning(pop))

#else

//   Other compilers probably have different pragmas for turning warnings off and on.
//   Adding more compilers to this list is fine, but the above probably capture most
//   developers and should be covered on dashboards.
#define SVTKM_DEPRECATED_SUPPRESS_BEGIN
#define SVTKM_DEPRECATED_SUPPRESS_END

#endif

#endif // SVTKM_DEPRECATED_SUPPRESS_SUPPORTED check

#if !defined(SVTKM_DEPRECATED_SUPPRESS_BEGIN) || !defined(SVTKM_DEPRECATED_SUPPRESS_END)
#error SVTKM_DEPRECATED_SUPPRESS macros not properly defined.
#endif

// Only actually use the [[deprecated]] attribute if the compiler supports it AND
// we know how to suppress deprecations when necessary.
#if defined(SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED) && defined(SVTKM_DEPRECATED_SUPPRESS_SUPPORTED)
#define SVTKM_DEPRECATED(...) [[deprecated(SVTK_M_DEPRECATED_MAKE_MESSAGE(__VA_ARGS__))]]
#else
#define SVTKM_DEPRECATED(...)
#endif

#endif // svtk_m_Deprecated_h
