//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ArrayPortalFromIterators_h
#define svtk_m_cont_internal_ArrayPortalFromIterators_h

#include <svtkm/Assert.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ArrayPortal.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/ErrorBadAllocation.h>

#include <iterator>
#include <limits>

#include <type_traits>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename IteratorT, typename Enable = void>
class ArrayPortalFromIterators;

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<!std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  ArrayPortalFromIterators() = default;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    SVTKM_ASSERT(numberOfValues >= 0);
#ifndef SVTKM_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<svtkm::Id>::max)())
    {
      throw svtkm::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on SVTKM_USE_64BIT_IDS.");
    }
#endif // !SVTKM_USE_64BIT_IDS
    this->NumberOfValues = static_cast<svtkm::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherIteratorT>
  SVTKM_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return *this->IteratorAt(index); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  void Set(svtkm::Id index, const ValueType& value) const { *(this->BeginIterator + index) = value; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    IteratorType iterator = this->BeginIterator;
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
  }

private:
  IteratorT BeginIterator;
  svtkm::Id NumberOfValues;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT IteratorAt(svtkm::Id index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};

template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalFromIterators()
    : BeginIterator(nullptr)
    , NumberOfValues(0)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    SVTKM_ASSERT(numberOfValues >= 0);
#ifndef SVTKM_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<svtkm::Id>::max)())
    {
      throw svtkm::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on SVTKM_USE_64BIT_IDS.");
    }
#endif // !SVTKM_USE_64BIT_IDS
    this->NumberOfValues = static_cast<svtkm::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherIteratorT>
  SVTKM_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return *this->IteratorAt(index); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  void Set(svtkm::Id svtkmNotUsed(index), const ValueType& svtkmNotUsed(value)) const
  {
#if !(defined(SVTKM_MSVC) && defined(SVTKM_CUDA))
    SVTKM_ASSERT(false && "Attempted to write to constant array.");
#endif
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    IteratorType iterator = this->BeginIterator;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
  }

private:
  IteratorT BeginIterator;
  svtkm::Id NumberOfValues;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  IteratorT IteratorAt(svtkm::Id index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_ArrayPortalFromIterators_h
