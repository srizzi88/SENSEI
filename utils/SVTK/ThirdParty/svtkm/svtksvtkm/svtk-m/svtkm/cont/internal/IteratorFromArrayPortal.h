//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_IteratorFromArrayPortal_h
#define svtk_m_cont_internal_IteratorFromArrayPortal_h

#include <svtkm/Assert.h>
#include <svtkm/cont/ArrayPortal.h>
#include <svtkm/internal/ArrayPortalValueReference.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <class ArrayPortalType>
class IteratorFromArrayPortal
{
public:
  using value_type = typename std::remove_const<typename ArrayPortalType::ValueType>::type;
  using reference = svtkm::internal::ArrayPortalValueReference<ArrayPortalType>;
  using pointer = typename std::add_pointer<value_type>::type;

  using difference_type = std::ptrdiff_t;

  using iterator_category = std::random_access_iterator_tag;

  using iter = IteratorFromArrayPortal<ArrayPortalType>;

  SVTKM_EXEC_CONT
  IteratorFromArrayPortal()
    : Portal()
    , Index(0)
  {
  }

  SVTKM_EXEC_CONT
  explicit IteratorFromArrayPortal(const ArrayPortalType& portal, svtkm::Id index = 0)
    : Portal(portal)
    , Index(index)
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index <= portal.GetNumberOfValues());
  }

  SVTKM_EXEC_CONT
  reference operator*() const { return reference(this->Portal, this->Index); }

  SVTKM_EXEC_CONT
  reference operator->() const { return reference(this->Portal, this->Index); }

  SVTKM_EXEC_CONT
  reference operator[](difference_type idx) const
  {
    return reference(this->Portal, this->Index + static_cast<svtkm::Id>(idx));
  }

  SVTKM_EXEC_CONT
  iter& operator++()
  {
    this->Index++;
    SVTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  SVTKM_EXEC_CONT
  iter operator++(int) { return iter(this->Portal, this->Index++); }

  SVTKM_EXEC_CONT
  iter& operator--()
  {
    this->Index--;
    SVTKM_ASSERT(this->Index >= 0);
    return *this;
  }

  SVTKM_EXEC_CONT
  iter operator--(int) { return iter(this->Portal, this->Index--); }

  SVTKM_EXEC_CONT
  iter& operator+=(difference_type n)
  {
    this->Index += static_cast<svtkm::Id>(n);
    SVTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  SVTKM_EXEC_CONT
  iter& operator-=(difference_type n)
  {
    this->Index += static_cast<svtkm::Id>(n);
    SVTKM_ASSERT(this->Index >= 0);
    return *this;
  }

  SVTKM_EXEC_CONT
  iter operator-(difference_type n) const
  {
    return iter(this->Portal, this->Index - static_cast<svtkm::Id>(n));
  }

  ArrayPortalType Portal;
  svtkm::Id Index;
};

template <class ArrayPortalType>
SVTKM_EXEC_CONT IteratorFromArrayPortal<ArrayPortalType> make_IteratorBegin(
  const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, 0);
}

template <class ArrayPortalType>
SVTKM_EXEC_CONT IteratorFromArrayPortal<ArrayPortalType> make_IteratorEnd(
  const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, portal.GetNumberOfValues());
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator==(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                               svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index == rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator!=(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                               svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index != rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator<(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                              svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index < rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator<=(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                               svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index <= rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator>(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                              svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index > rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT bool operator>=(svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                               svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index >= rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT std::ptrdiff_t operator-(
  svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index - rhs.Index;
}

template <typename PortalType>
SVTKM_EXEC_CONT svtkm::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& iter,
  std::ptrdiff_t n)
{
  return svtkm::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<svtkm::Id>(n));
}

template <typename PortalType>
SVTKM_EXEC_CONT svtkm::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  std::ptrdiff_t n,
  svtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& iter)
{
  return svtkm::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<svtkm::Id>(n));
}
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_IteratorFromArrayPortal_h
