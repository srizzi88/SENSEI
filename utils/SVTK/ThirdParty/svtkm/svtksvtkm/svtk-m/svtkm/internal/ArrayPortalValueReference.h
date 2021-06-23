//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_ArrayPortalValueReference_h
#define svtk_m_internal_ArrayPortalValueReference_h

#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace internal
{

/// \brief A value class for returning setable values of an ArrayPortal
///
/// \c ArrayPortal classes have a pair of \c Get and \c Set methods that
/// retrieve and store values in the array. This is to make it easy to
/// implement the \c ArrayPortal even it is not really an array. However, there
/// are some cases where the code structure expects a reference to a value that
/// can be set. For example, the \c IteratorFromArrayPortal class must return
/// something from its * operator that behaves like a reference.
///
/// For cases of this nature \c ArrayPortalValueReference can be used. This
/// class is constructured with an \c ArrayPortal and an index into the array.
/// The object then behaves like a reference to the value in the array. If you
/// set this reference object to a new value, it will call \c Set on the
/// \c ArrayPortal to insert the value into the array.
///
template <typename ArrayPortalType>
struct ArrayPortalValueReference
{
  using ValueType = typename ArrayPortalType::ValueType;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalValueReference(const ArrayPortalType& portal, svtkm::Id index)
    : Portal(portal)
    , Index(index)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalValueReference(const ArrayPortalValueReference& ref)
    : Portal(ref.Portal)
    , Index(ref.Index)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get() const { return this->Portal.Get(this->Index); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  operator ValueType(void) const { return this->Get(); }

  // Declaring Set as const seems a little weird because we are changing the value. But remember
  // that ArrayPortalReference is only a reference class. The reference itself does not change,
  // just the thing that it is referencing. So declaring as const is correct and necessary so that
  // you can set the value of a reference returned from a function (which is a right hand side
  // value).

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  void Set(ValueType&& value) const { this->Portal.Set(this->Index, std::move(value)); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  void Set(const ValueType& value) const { this->Portal.Set(this->Index, value); }

  SVTKM_CONT
  void Swap(const ArrayPortalValueReference<ArrayPortalType>& rhs) const noexcept
  {
    //we need use the explicit type not a proxy temp object
    //A proxy temp object would point to the same underlying data structure
    //and would not hold the old value of *this once *this was set to rhs.
    const ValueType aValue = *this;
    *this = rhs;
    rhs = aValue;
  }

  // Declaring operator= as const seems a little weird because we are changing the value. But
  // remember that ArrayPortalReference is only a reference class. The reference itself does not
  // change, just the thing that it is referencing. So declaring as const is correct and necessary
  // so that you can set the value of a reference returned from a function (which is a right hand
  // side value).

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const ArrayPortalValueReference<ArrayPortalType>& operator=(ValueType&& value) const
  {
    this->Set(std::move(value));
    return *this;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const ArrayPortalValueReference<ArrayPortalType>& operator=(const ValueType& value) const
  {
    this->Set(value);
    return *this;
  }

  // This special overload of the operator= is to override the behavior of the default operator=
  // (which has the wrong behavior) with behavior consistent with the other operator= methods.
  // It is not declared const because the default operator= is not declared const. If you try
  // to do this assignment with a const object, you will get one of the operator= above.
  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT ArrayPortalValueReference<ArrayPortalType>& operator=(
    const ArrayPortalValueReference<ArrayPortalType>& rhs)
  {
    this->Set(static_cast<ValueType>(rhs.Portal.Get(rhs.Index)));
    return *this;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator+=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs += rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator+=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs += rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator-=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs -= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator-=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs -= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator*=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs *= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator*=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs *= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator/=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs /= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator/=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs /= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator%=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs %= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator%=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs %= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator&=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs &= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator&=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs &= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator|=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs |= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator|=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs |= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator^=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs ^= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator^=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs ^= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator>>=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs >>= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator>>=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs >>= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator<<=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs <<= rhs;
    this->Set(lhs);
    return lhs;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  SVTKM_EXEC_CONT ValueType operator<<=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs <<= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

private:
  const ArrayPortalType& Portal;
  svtkm::Id Index;
};

//implement a custom swap function, since the std::swap won't work
//since we return RValues instead of Lvalues
template <typename T>
void swap(const svtkm::internal::ArrayPortalValueReference<T>& a,
          const svtkm::internal::ArrayPortalValueReference<T>& b)
{
  a.Swap(b);
}

template <typename T>
void swap(const svtkm::internal::ArrayPortalValueReference<T>& a,
          typename svtkm::internal::ArrayPortalValueReference<T>::ValueType& b)
{
  using ValueType = typename svtkm::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = a;
  a = b;
  b = tmp;
}

template <typename T>
void swap(typename svtkm::internal::ArrayPortalValueReference<T>::ValueType& a,
          const svtkm::internal::ArrayPortalValueReference<T>& b)
{
  using ValueType = typename svtkm::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = b;
  b = a;
  a = tmp;
}

// The reason why all the operators on ArrayPortalValueReference are defined outside of the class
// is so that in the case that the operator in question is not defined in the value type, these
// operators will not be instantiated (and therefore cause a compile error) unless they are
// directly used (in which case a compile error is appropriate).

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator==(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() == rhs)
{
  return lhs.Get() == rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator==(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() == rhs.Get())
{
  return lhs.Get() == rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator==(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs == rhs.Get())
{
  return lhs == rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator!=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() != rhs)
{
  return lhs.Get() != rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator!=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() != rhs.Get())
{
  return lhs.Get() != rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator!=(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs != rhs.Get())
{
  return lhs != rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() < rhs)
{
  return lhs.Get() < rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() < rhs.Get())
{
  return lhs.Get() < rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs < rhs.Get())
{
  return lhs < rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() > rhs)
{
  return lhs.Get() > rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() > rhs.Get())
{
  return lhs.Get() > rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs > rhs.Get())
{
  return lhs > rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator<=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() <= rhs)
{
  return lhs.Get() <= rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() <= rhs.Get())
{
  return lhs.Get() <= rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<=(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs <= rhs.Get())
{
  return lhs <= rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator>=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() >= rhs)
{
  return lhs.Get() >= rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() >= rhs.Get())
{
  return lhs.Get() >= rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>=(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs >= rhs.Get())
{
  return lhs >= rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator+(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() + rhs)
{
  return lhs.Get() + rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator+(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() + rhs.Get())
{
  return lhs.Get() + rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator+(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs + rhs.Get())
{
  return lhs + rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator-(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() - rhs)
{
  return lhs.Get() - rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator-(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() - rhs.Get())
{
  return lhs.Get() - rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator-(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs - rhs.Get())
{
  return lhs - rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator*(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() * rhs)
{
  return lhs.Get() * rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator*(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() * rhs.Get())
{
  return lhs.Get() * rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator*(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs * rhs.Get())
{
  return lhs * rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator/(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() / rhs)
{
  return lhs.Get() / rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator/(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() / rhs.Get())
{
  return lhs.Get() / rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator/(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs / rhs.Get())
{
  return lhs / rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator%(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() % rhs)
{
  return lhs.Get() % rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator%(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() % rhs.Get())
{
  return lhs.Get() % rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator%(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs % rhs.Get())
{
  return lhs % rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator^(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() ^ rhs)
{
  return lhs.Get() ^ rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator^(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() ^ rhs.Get())
{
  return lhs.Get() ^ rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator^(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs ^ rhs.Get())
{
  return lhs ^ rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator|(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() | rhs)
{
  return lhs.Get() | rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator|(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() | rhs.Get())
{
  return lhs.Get() | rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator|(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs | rhs.Get())
{
  return lhs | rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() & rhs)
{
  return lhs.Get() & rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() & rhs.Get())
{
  return lhs.Get() & rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator&(const typename RhsPortalType::ValueType& lhs,
                              const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs & rhs.Get())
{
  return lhs & rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator<<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() << rhs)
{
  return lhs.Get() << rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() << rhs.Get())
{
  return lhs.Get() << rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator<<(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs << rhs.Get())
{
  return lhs << rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator>>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() >> rhs)
{
  return lhs.Get() >> rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() >> rhs.Get())
{
  return lhs.Get() >> rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator>>(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs >> rhs.Get())
{
  return lhs >> rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
SVTKM_EXEC_CONT auto operator~(const ArrayPortalValueReference<PortalType>& ref)
  -> decltype(~ref.Get())
{
  return ~ref.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
SVTKM_EXEC_CONT auto operator!(const ArrayPortalValueReference<PortalType>& ref)
  -> decltype(!ref.Get())
{
  return !ref.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator&&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() && rhs)
{
  return lhs.Get() && rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator&&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() && rhs.Get())
{
  return lhs.Get() && rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator&&(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs && rhs.Get())
{
  return lhs && rhs.Get();
}

SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
SVTKM_EXEC_CONT auto operator||(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() || rhs)
{
  return lhs.Get() || rhs;
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
SVTKM_EXEC_CONT auto operator||(const ArrayPortalValueReference<LhsPortalType>& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() || rhs.Get())
{
  return lhs.Get() || rhs.Get();
}
SVTKM_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
SVTKM_EXEC_CONT auto operator||(const typename RhsPortalType::ValueType& lhs,
                               const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs || rhs.Get())
{
  return lhs || rhs.Get();
}
}
} // namespace svtkm::internal

namespace svtkm
{

// Make specialization for TypeTraits and VecTraits so that the reference
// behaves the same as the value.

template <typename PortalType>
struct TypeTraits<svtkm::internal::ArrayPortalValueReference<PortalType>>
  : svtkm::TypeTraits<typename svtkm::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

template <typename PortalType>
struct VecTraits<svtkm::internal::ArrayPortalValueReference<PortalType>>
  : svtkm::VecTraits<typename svtkm::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

} // namespace svtkm

#endif //svtk_m_internal_ArrayPortalValueReference_h
