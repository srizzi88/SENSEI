//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_TypeTraits_h
#define svtk_m_TypeTraits_h

#include <svtkm/Types.h>

namespace svtkm
{

/// Tag used to identify types that aren't Real, Integer, Scalar or Vector.
///
struct TypeTraitsUnknownTag
{
};

/// Tag used to identify types that store real (floating-point) numbers. A
/// TypeTraits class will typedef this class to NumericTag if it stores real
/// numbers (or vectors of real numbers).
///
struct TypeTraitsRealTag
{
};

/// Tag used to identify types that store integer numbers. A TypeTraits class
/// will typedef this class to NumericTag if it stores integer numbers (or
/// vectors of integers).
///
struct TypeTraitsIntegerTag
{
};

/// Tag used to identify 0 dimensional types (scalars). Scalars can also be
/// treated like vectors when used with VecTraits. A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsScalarTag
{
};

/// Tag used to identify 1 dimensional types (vectors). A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsVectorTag
{
};

/// The TypeTraits class provides helpful compile-time information about the
/// basic types used in SVTKm (and a few others for convenience). The majority
/// of TypeTraits contents are typedefs to tags that can be used to easily
/// override behavior of called functions.
///
template <typename T>
class TypeTraits
{
public:
  /// \brief A tag to determine whether the type is integer or real.
  ///
  /// This tag is either TypeTraitsRealTag or TypeTraitsIntegerTag.
  using NumericTag = svtkm::TypeTraitsUnknownTag;

  /// \brief A tag to determine whether the type has multiple components.
  ///
  /// This tag is either TypeTraitsScalarTag or TypeTraitsVectorTag. Scalars can
  /// also be treated as vectors.
  using DimensionalityTag = svtkm::TypeTraitsUnknownTag;

  SVTKM_EXEC_CONT static T ZeroInitialization() { return T(); }
};

// Const types should have the same traits as their non-const counterparts.
//
template <typename T>
struct TypeTraits<const T> : TypeTraits<T>
{
};

#define SVTKM_BASIC_REAL_TYPE(T)                                                                    \
  template <>                                                                                      \
  struct TypeTraits<T>                                                                             \
  {                                                                                                \
    using NumericTag = TypeTraitsRealTag;                                                          \
    using DimensionalityTag = TypeTraitsScalarTag;                                                 \
    SVTKM_EXEC_CONT static T ZeroInitialization() { return T(); }                                   \
  };

#define SVTKM_BASIC_INTEGER_TYPE(T)                                                                 \
  template <>                                                                                      \
  struct TypeTraits<T>                                                                             \
  {                                                                                                \
    using NumericTag = TypeTraitsIntegerTag;                                                       \
    using DimensionalityTag = TypeTraitsScalarTag;                                                 \
    SVTKM_EXEC_CONT static T ZeroInitialization()                                                   \
    {                                                                                              \
      using ReturnType = T;                                                                        \
      return ReturnType();                                                                         \
    }                                                                                              \
  };

/// Traits for basic C++ types.
///

SVTKM_BASIC_REAL_TYPE(float)
SVTKM_BASIC_REAL_TYPE(double)

SVTKM_BASIC_INTEGER_TYPE(char)
SVTKM_BASIC_INTEGER_TYPE(signed char)
SVTKM_BASIC_INTEGER_TYPE(unsigned char)
SVTKM_BASIC_INTEGER_TYPE(short)
SVTKM_BASIC_INTEGER_TYPE(unsigned short)
SVTKM_BASIC_INTEGER_TYPE(int)
SVTKM_BASIC_INTEGER_TYPE(unsigned int)
SVTKM_BASIC_INTEGER_TYPE(long)
SVTKM_BASIC_INTEGER_TYPE(unsigned long)
SVTKM_BASIC_INTEGER_TYPE(long long)
SVTKM_BASIC_INTEGER_TYPE(unsigned long long)

#undef SVTKM_BASIC_REAL_TYPE
#undef SVTKM_BASIC_INTEGER_TYPE

/// Traits for Vec types.
///
template <typename T, svtkm::IdComponent Size>
struct TypeTraits<svtkm::Vec<T, Size>>
{
  using NumericTag = typename svtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = svtkm::TypeTraitsVectorTag;

  SVTKM_EXEC_CONT
  static svtkm::Vec<T, Size> ZeroInitialization()
  {
    return svtkm::Vec<T, Size>(svtkm::TypeTraits<T>::ZeroInitialization());
  }
};

/// Traits for VecCConst types.
///
template <typename T>
struct TypeTraits<svtkm::VecCConst<T>>
{
  using NumericTag = typename svtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_EXEC_CONT
  static svtkm::VecCConst<T> ZeroInitialization() { return svtkm::VecCConst<T>(); }
};

/// Traits for VecC types.
///
template <typename T>
struct TypeTraits<svtkm::VecC<T>>
{
  using NumericTag = typename svtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_EXEC_CONT
  static svtkm::VecC<T> ZeroInitialization() { return svtkm::VecC<T>(); }
};

/// \brief Traits for Pair types.
///
template <typename T, typename U>
struct TypeTraits<svtkm::Pair<T, U>>
{
  using NumericTag = svtkm::TypeTraitsUnknownTag;
  using DimensionalityTag = svtkm::TypeTraitsScalarTag;

  SVTKM_EXEC_CONT
  static svtkm::Pair<T, U> ZeroInitialization()
  {
    return svtkm::Pair<T, U>(TypeTraits<T>::ZeroInitialization(),
                            TypeTraits<U>::ZeroInitialization());
  }
};

} // namespace svtkm

#endif //svtk_m_TypeTraits_h
