//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_cuda_internal_WrappedOperators_h
#define svtk_m_exec_cuda_internal_WrappedOperators_h

#include <svtkm/BinaryPredicates.h>
#include <svtkm/Pair.h>
#include <svtkm/Types.h>
#include <svtkm/exec/cuda/internal/IteratorFromArrayPortal.h>
#include <svtkm/internal/ExportMacros.h>

// Disable warnings we check svtkm for but Thrust does not.
#include <svtkm/exec/cuda/internal/ThrustPatches.h>
SVTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
SVTKM_THIRDPARTY_POST_INCLUDE

namespace svtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

// Unary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedUnaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects unary operators to have
  using first_argument_type = T;
  using result_type = bool;

  Function m_f;

  SVTKM_EXEC
  WrappedUnaryPredicate()
    : m_f()
  {
  }

  SVTKM_CONT
  WrappedUnaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  SVTKM_EXEC bool operator()(const T& x) const { return m_f(x); }

  template <typename U>
  SVTKM_EXEC bool operator()(const svtkm::internal::ArrayPortalValueReference<U>& x) const
  {
    return m_f(x.Get());
  }

  SVTKM_EXEC bool operator()(const T* x) const { return m_f(*x); }
};

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedBinaryOperator
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = T;

  Function m_f;

  SVTKM_EXEC
  WrappedBinaryOperator()
    : m_f()
  {
  }

  SVTKM_CONT
  WrappedBinaryOperator(const Function& f)
    : m_f(f)
  {
  }

  SVTKM_EXEC T operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  SVTKM_EXEC T operator()(const T& x, const svtkm::internal::ArrayPortalValueReference<U>& y) const
  {
    // to support proper implicit conversion, and avoid overload
    // ambiguities.
    return m_f(x, y.Get());
  }

  template <typename U>
  SVTKM_EXEC T operator()(const svtkm::internal::ArrayPortalValueReference<U>& x, const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  SVTKM_EXEC T operator()(const svtkm::internal::ArrayPortalValueReference<U>& x,
                         const svtkm::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  SVTKM_EXEC T operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  SVTKM_EXEC T operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  SVTKM_EXEC T operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};

template <typename T_, typename Function>
struct WrappedBinaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = bool;

  Function m_f;

  SVTKM_EXEC
  WrappedBinaryPredicate()
    : m_f()
  {
  }

  SVTKM_CONT
  WrappedBinaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  SVTKM_EXEC bool operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  SVTKM_EXEC bool operator()(const T& x, const svtkm::internal::ArrayPortalValueReference<U>& y) const
  {
    return m_f(x, y.Get());
  }

  template <typename U>
  SVTKM_EXEC bool operator()(const svtkm::internal::ArrayPortalValueReference<U>& x, const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  SVTKM_EXEC bool operator()(const svtkm::internal::ArrayPortalValueReference<U>& x,
                            const svtkm::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  SVTKM_EXEC bool operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  SVTKM_EXEC bool operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  SVTKM_EXEC bool operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};
}
}
}
} //namespace svtkm::exec::cuda::internal

namespace thrust
{
namespace detail
{
//
// We tell Thrust that our WrappedBinaryOperator is commutative so that we
// activate numerous fast paths inside thrust which are only available when
// the binary functor is commutative and the T type is is_arithmetic
//
//
template <typename T, typename F>
struct is_commutative<svtkm::exec::cuda::internal::WrappedBinaryOperator<T, F>>
  : public thrust::detail::is_arithmetic<T>
{
};
}
} //namespace thrust::detail

#endif //svtk_m_exec_cuda_internal_WrappedOperators_h
