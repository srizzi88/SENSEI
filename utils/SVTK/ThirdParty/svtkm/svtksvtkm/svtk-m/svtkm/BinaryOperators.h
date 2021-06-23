//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_BinaryOperators_h
#define svtk_m_BinaryOperators_h

#include <svtkm/Math.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{

// Disable conversion warnings for Sum and Product on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns sum (addition) of the two values.
/// @note Requires a suitable definition of `operator+(T, U)`.
struct Sum
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x + y)
  {
    return x + y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  SVTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x + y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns product (multiplication) of the two values.
/// @note Requires a suitable definition of `operator*(T, U)`.
struct Product
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x * y)
  {
    return x * y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  SVTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x * y);
  }
};

#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x > y otherwise returns \c y.
/// @note Requires a suitable definition of `bool operator<(T, U)` and that
/// `T` and `U` share a common type.
//needs to be full length to not clash with svtkm::math function Max.
struct Maximum
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT typename std::common_type<T, U>::type operator()(const T& x, const U& y) const
  {
    return x < y ? y : x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x < y otherwise returns \c y.
/// @note Requires a suitable definition of `bool operator<(T, U)` and that
/// `T` and `U` share a common type.
//needs to be full length to not clash with svtkm::math function Min.
struct Minimum
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT typename std::common_type<T, U>::type operator()(const T& x, const U& y) const
  {
    return x < y ? x : y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns a svtkm::Vec<T,2> that represents the minimum and maximum values.
/// Note: Requires Type \p T implement the svtkm::Min and svtkm::Max functions.
template <typename T>
struct MinAndMax
{
  SVTKM_EXEC_CONT
  svtkm::Vec<T, 2> operator()(const T& a) const { return svtkm::make_Vec(a, a); }

  SVTKM_EXEC_CONT
  svtkm::Vec<T, 2> operator()(const T& a, const T& b) const
  {
    return svtkm::make_Vec(svtkm::Min(a, b), svtkm::Max(a, b));
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<T, 2> operator()(const svtkm::Vec<T, 2>& a, const svtkm::Vec<T, 2>& b) const
  {
    return svtkm::make_Vec(svtkm::Min(a[0], b[0]), svtkm::Max(a[1], b[1]));
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<T, 2> operator()(const T& a, const svtkm::Vec<T, 2>& b) const
  {
    return svtkm::make_Vec(svtkm::Min(a, b[0]), svtkm::Max(a, b[1]));
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<T, 2> operator()(const svtkm::Vec<T, 2>& a, const T& b) const
  {
    return svtkm::make_Vec(svtkm::Min(a[0], b), svtkm::Max(a[1], b));
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x&y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseAnd
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x & y)
  {
    return x & y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  SVTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x & y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x|y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseOr
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x | y)
  {
    return x | y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  SVTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x | y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x^y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseXor
{
  template <typename T, typename U>
  SVTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x ^ y)
  {
    return x ^ y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  SVTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x ^ y);
  }
};

} // namespace svtkm

#endif //svtk_m_BinaryOperators_h
