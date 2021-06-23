//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_ArrayPortalHelpers_h
#define svtk_m_internal_ArrayPortalHelpers_h


#include <svtkm/VecTraits.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{
namespace internal
{

namespace detail
{

template <typename PortalType>
struct PortalSupportsGetsImpl
{
  template <typename U, typename S = decltype(std::declval<U>().Get(svtkm::Id{}))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsSetsImpl
{
  template <typename U,
            typename S = decltype(std::declval<U>().Set(svtkm::Id{},
                                                        std::declval<typename U::ValueType>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

} // namespace detail

template <typename PortalType>
using PortalSupportsGets =
  typename detail::PortalSupportsGetsImpl<typename std::decay<PortalType>::type>::type;

template <typename PortalType>
using PortalSupportsSets =
  typename detail::PortalSupportsSetsImpl<typename std::decay<PortalType>::type>::type;
}
} // namespace svtkm::internal

#endif //svtk_m_internal_ArrayPortalHelpers_h
