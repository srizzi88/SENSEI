//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VectorAnalysis_h
#define svtk_m_VectorAnalysis_h

// This header file defines math functions that deal with linear albegra functions

#include <svtkm/Math.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{

// ----------------------------------------------------------------------------
/// \brief Returns the linear interpolation of two values based on weight
///
/// \c Lerp interpolates return the linerar interpolation of v0 and v1 based on w. v0
/// and v1 are scalars or vectors of same length. w can either be a scalar or a
/// vector of the same length as x and y. If w is outside [0,1] then lerp
/// extrapolates. If w=0 => v0 is returned if w=1 => v1 is returned.
///
template <typename ValueType, typename WeightType>
inline SVTKM_EXEC_CONT ValueType Lerp(const ValueType& value0,
                                     const ValueType& value1,
                                     const WeightType& weight)
{
  using ScalarType = typename detail::FloatingPointReturnType<ValueType>::Type;
  return static_cast<ValueType>((WeightType(1) - weight) * static_cast<ScalarType>(value0) +
                                weight * static_cast<ScalarType>(value1));
}
template <typename ValueType, svtkm::IdComponent N, typename WeightType>
SVTKM_EXEC_CONT svtkm::Vec<ValueType, N> Lerp(const svtkm::Vec<ValueType, N>& value0,
                                            const svtkm::Vec<ValueType, N>& value1,
                                            const WeightType& weight)
{
  return (WeightType(1) - weight) * value0 + weight * value1;
}
template <typename ValueType, svtkm::IdComponent N>
SVTKM_EXEC_CONT svtkm::Vec<ValueType, N> Lerp(const svtkm::Vec<ValueType, N>& value0,
                                            const svtkm::Vec<ValueType, N>& value1,
                                            const svtkm::Vec<ValueType, N>& weight)
{
  const svtkm::Vec<ValueType, N> One(ValueType(1));
  return (One - weight) * value0 + weight * value1;
}

// ----------------------------------------------------------------------------
/// \brief Returns the square of the magnitude of a vector.
///
/// It is usually much faster to compute the square of the magnitude than the
/// square, so you should use this function in place of Magnitude or RMagnitude
/// when possible.
///
template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeSquared(const T& x)
{
  using U = typename detail::FloatingPointReturnType<T>::Type;
  return static_cast<U>(svtkm::Dot(x, x));
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  T x,
  svtkm::TypeTraitsScalarTag)
{
  return static_cast<typename detail::FloatingPointReturnType<T>::Type>(svtkm::Abs(x));
}

template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  const T& x,
  svtkm::TypeTraitsVectorTag)
{
  return svtkm::Sqrt(svtkm::MagnitudeSquared(x));
}

} // namespace detail

/// \brief Returns the magnitude of a vector.
///
/// It is usually much faster to compute MagnitudeSquared, so that should be
/// substituted when possible (unless you are just going to take the square
/// root, which would be besides the point). On some hardware it is also faster
/// to find the reciprocal magnitude, so RMagnitude should be used if you
/// actually plan to divide by the magnitude.
///
template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type Magnitude(const T& x)
{
  return detail::MagnitudeTemplate(x, typename svtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  T x,
  svtkm::TypeTraitsScalarTag)
{
  return T(1) / svtkm::Abs(x);
}

template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  const T& x,
  svtkm::TypeTraitsVectorTag)
{
  return svtkm::RSqrt(svtkm::MagnitudeSquared(x));
}
} // namespace detail

/// \brief Returns the reciprocal magnitude of a vector.
///
/// On some hardware RMagnitude is faster than Magnitude, but neither is
/// as fast as MagnitudeSquared.
///
template <typename T>
SVTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitude(const T& x)
{
  return detail::RMagnitudeTemplate(x, typename svtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
SVTKM_EXEC_CONT T NormalTemplate(T x, svtkm::TypeTraitsScalarTag)
{
  return svtkm::CopySign(T(1), x);
}

template <typename T>
SVTKM_EXEC_CONT T NormalTemplate(const T& x, svtkm::TypeTraitsVectorTag)
{
  return svtkm::RMagnitude(x) * x;
}
} // namespace detail

/// \brief Returns a normalized version of the given vector.
///
/// The resulting vector points in the same direction but has unit length.
///
template <typename T>
SVTKM_EXEC_CONT T Normal(const T& x)
{
  return detail::NormalTemplate(x, typename svtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
/// \brief Changes a vector to be normal.
///
/// The given vector is scaled to be unit length.
///
template <typename T>
SVTKM_EXEC_CONT void Normalize(T& x)
{
  x = svtkm::Normal(x);
}

// ----------------------------------------------------------------------------
/// \brief Find the cross product of two vectors.
///
template <typename T>
SVTKM_EXEC_CONT svtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3> Cross(
  const svtkm::Vec<T, 3>& x,
  const svtkm::Vec<T, 3>& y)
{
  return svtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>(
    x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]);
}

//-----------------------------------------------------------------------------
/// \brief Find the normal of a triangle.
///
/// Given three coordinates in space, which, unless degenerate, uniquely define
/// a triangle and the plane the triangle is on, returns a vector perpendicular
/// to that triangle/plane.
///
/// Note that the returned vector might not be a unit vector. In fact, the length
/// is equal to twice the area of the triangle. If you want a unit vector,
/// send the result through the \c Normal function.
///
template <typename T>
SVTKM_EXEC_CONT svtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>
TriangleNormal(const svtkm::Vec<T, 3>& a, const svtkm::Vec<T, 3>& b, const svtkm::Vec<T, 3>& c)
{
  return svtkm::Cross(b - a, c - a);
}

//-----------------------------------------------------------------------------
/// \brief Project a vector onto another vector.
///
/// This method computes the orthogonal projection of the vector v onto u;
/// that is, it projects its first argument onto its second.
///
/// Note that if the vector \a u has zero length, the output
/// vector will have all its entries equal to NaN.
template <typename T, int N>
SVTKM_EXEC_CONT svtkm::Vec<T, N> Project(const svtkm::Vec<T, N>& v, const svtkm::Vec<T, N>& u)
{
  T uu = svtkm::Dot(u, u);
  T uv = svtkm::Dot(u, v);
  T factor = uv / uu;
  svtkm::Vec<T, N> result = factor * u;
  return result;
}

//-----------------------------------------------------------------------------
/// \brief Project a vector onto another vector, returning only the projected distance.
///
/// This method computes the orthogonal projection of the vector v onto u;
/// that is, it projects its first argument onto its second.
///
/// Note that if the vector \a u has zero length, the output will be NaN.
template <typename T, int N>
SVTKM_EXEC_CONT T ProjectedDistance(const svtkm::Vec<T, N>& v, const svtkm::Vec<T, N>& u)
{
  T uu = svtkm::Dot(u, u);
  T uv = svtkm::Dot(u, v);
  T factor = uv / uu;
  return factor;
}

//-----------------------------------------------------------------------------
/// \brief Perform Gram-Schmidt orthonormalization for 3-D vectors.
///
/// See https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process for details.
/// The first output vector will always be parallel to the first input vector.
/// The remaining output vectors will be orthogonal and unit length and have
/// the same handedness as their corresponding input vectors.
///
/// This method is geometric.
/// It does not require a matrix solver.
/// However, unlike the algebraic eigensolver techniques which do use matrix
/// inversion, this method may return zero-length output vectors if some input
/// vectors are collinear. The number of non-zero (to within the specified
/// tolerance, \a tol ) output vectors is the return value.
template <typename T, int N>
SVTKM_EXEC_CONT int Orthonormalize(const svtkm::Vec<svtkm::Vec<T, N>, N>& inputs,
                                  svtkm::Vec<svtkm::Vec<T, N>, N>& outputs,
                                  T tol = static_cast<T>(1e-6))
{
  int j = 0; // j is the number of non-zero-length, non-collinear inputs encountered.
  svtkm::Vec<svtkm::Vec<T, N>, N> u;
  for (int i = 0; i < N; ++i)
  {
    u[j] = inputs[i];
    for (int k = 0; k < j; ++k)
    {
      u[j] -= svtkm::Project(inputs[i], u[k]);
    }
    T rmag = svtkm::RMagnitude(u[j]);
    if (rmag * tol > 1.0)
    {
      // skip this vector, it is zero-length or collinear with others.
      continue;
    }
    outputs[j] = rmag * u[j];
    ++j;
  }
  for (int i = j; i < N; ++i)
  {
    outputs[j] = Vec<T, N>{ 0. };
  }
  return j;
}

} // namespace svtkm

#endif //svtk_m_VectorAnalysis_h
