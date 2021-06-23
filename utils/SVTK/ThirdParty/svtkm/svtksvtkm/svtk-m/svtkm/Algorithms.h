//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Algorithms_h
#define svtk_m_Algorithms_h

#include <svtkm/cont/ArrayPortalToIterators.h>

#include <svtkm/BinaryPredicates.h>

#include <svtkm/internal/Configure.h>

#include <algorithm>
#include <iterator>

namespace svtkm
{

/// Similar to std::lower_bound and std::upper_bound, but returns an iterator
/// to any matching item (rather than a specific one). Returns @a last when
/// @a val is not found.
/// @{
template <typename IterT, typename T, typename Comp>
SVTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val, Comp comp)
{
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (comp(*mid, val))
    {
      first = mid + 1;
      len -= halfLen + 1;
    }
    else if (comp(val, *mid))
    {
      len = halfLen;
    }
    else
    {
      return mid; // found element
    }
  }
  return last; // did not find element
}

template <typename IterT, typename T>
SVTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val)
{
  return svtkm::BinarySearch(first, last, val, svtkm::SortLess{});
}
/// @}

/// Similar to std::lower_bound and std::upper_bound, but returns the index of
/// any matching item (rather than a specific one). Returns -1 when @a val is not
/// found.
/// @{
template <typename PortalT, typename T, typename Comp>
SVTKM_EXEC_CONT svtkm::Id BinarySearch(const PortalT& portal, const T& val, Comp comp)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::BinarySearch(first, last, val, comp);
  return result == last ? static_cast<svtkm::Id>(-1) : static_cast<svtkm::Id>(result - first);
}

// Return -1 if not found
template <typename PortalT, typename T>
SVTKM_EXEC_CONT svtkm::Id BinarySearch(const PortalT& portal, const T& val)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::BinarySearch(first, last, val, svtkm::SortLess{});
  return result == last ? static_cast<svtkm::Id>(-1) : static_cast<svtkm::Id>(result - first);
}
/// @}

/// Implementation of std::lower_bound or std::upper_bound that is appropriate
/// for both control and execution environments.
/// The overloads that take portals return indices instead of iterators.
/// @{
template <typename IterT, typename T, typename Comp>
SVTKM_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val, Comp comp)
{
#ifdef SVTKM_CUDA
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (comp(*mid, val))
    {
      first = mid + 1;
      len -= halfLen + 1;
    }
    else
    {
      len = halfLen;
    }
  }
  return first;
#else  // SVTKM_CUDA
  return std::lower_bound(first, last, val, std::move(comp));
#endif // SVTKM_CUDA
}

template <typename IterT, typename T>
SVTKM_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val)
{
  return svtkm::LowerBound(first, last, val, svtkm::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
SVTKM_EXEC_CONT svtkm::Id LowerBound(const PortalT& portal, const T& val, Comp comp)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::LowerBound(first, last, val, comp);
  return static_cast<svtkm::Id>(result - first);
}

template <typename PortalT, typename T>
SVTKM_EXEC_CONT svtkm::Id LowerBound(const PortalT& portal, const T& val)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::LowerBound(first, last, val, svtkm::SortLess{});
  return static_cast<svtkm::Id>(result - first);
}

template <typename IterT, typename T, typename Comp>
SVTKM_EXEC_CONT IterT UpperBound(IterT first, IterT last, const T& val, Comp comp)
{
#ifdef SVTKM_CUDA
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (!comp(val, *mid))
    {
      first = mid + 1;
      len -= halfLen + 1;
    }
    else
    {
      len = halfLen;
    }
  }
  return first;
#else  // SVTKM_CUDA
  return std::upper_bound(first, last, val, std::move(comp));
#endif // SVTKM_CUDA
}

template <typename IterT, typename T>
SVTKM_EXEC_CONT IterT UpperBound(IterT first, IterT last, const T& val)
{
  return svtkm::UpperBound(first, last, val, svtkm::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
SVTKM_EXEC_CONT svtkm::Id UpperBound(const PortalT& portal, const T& val, Comp comp)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::UpperBound(first, last, val, comp);
  return static_cast<svtkm::Id>(result - first);
}

template <typename PortalT, typename T>
SVTKM_EXEC_CONT svtkm::Id UpperBound(const PortalT& portal, const T& val)
{
  auto first = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = svtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = svtkm::UpperBound(first, last, val, svtkm::SortLess{});
  return static_cast<svtkm::Id>(result - first);
}
/// @}

} // end namespace svtkm

#endif // svtk_m_Algorithms_h
