//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_ListTag_h
#define svtk_m_ListTag_h

#include <svtkm/Deprecated.h>

#include <svtkm/List.h>

#include <svtkm/StaticAssert.h>
#include <svtkm/internal/ExportMacros.h>
#include <svtkm/internal/brigand.hpp>

#include <type_traits>

namespace svtkm
{
namespace detail
{

//-----------------------------------------------------------------------------

/// Base class that all ListTag classes inherit from. Helps identify lists
/// in macros like SVTKM_IS_LIST_TAG.
///
struct ListRoot
{
};

template <class... T>
using ListBase = brigand::list<T...>;

/// list value that is used to represent a list actually matches all values
struct UniversalTag
{
  //We never want this tag constructed, and by deleting the constructor
  //we get an error when trying to use this class with ForEach.
  UniversalTag() = delete;
};

} // namespace detail

//-----------------------------------------------------------------------------
/// A basic tag for a list of typenames. This struct can be subclassed
/// and still behave like a list tag.
template <typename... ArgTypes>
struct SVTKM_DEPRECATED(1.6, "ListTagBase replace by List. Note that List cannot be subclassed.")
  ListTagBase : detail::ListRoot
{
  using list = detail::ListBase<ArgTypes...>;
};

/// A special tag for a list that represents holding all potential values
///
/// Note: Can not be used with ForEach for obvious reasons.
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagUniversal replaced by ListUniversal. Note that ListUniversal cannot be subclassed.")
  ListTagUniversal : detail::ListRoot
{
  using list = svtkm::detail::ListBase<svtkm::detail::UniversalTag>;
};

namespace internal
{

template <typename ListTag>
struct ListTagCheck : std::is_base_of<svtkm::detail::ListRoot, ListTag>
{
  static constexpr bool Valid = std::is_base_of<svtkm::detail::ListRoot, ListTag>::value;
};

} // namespace internal

namespace detail
{

template <typename ListTag>
struct SVTKM_DEPRECATED(1.6, "SVTKM_IS_LIST_TAG replaced with SVTKM_IS_LIST.") ListTagAssert
  : internal::IsList<ListTag>
{
};

} // namespace detal

/// Checks that the argument is a proper list tag. This is a handy concept
/// check for functions and classes to make sure that a template argument is
/// actually a device adapter tag. (You can get weird errors elsewhere in the
/// code when a mistake is made.)
///
#define SVTKM_IS_LIST_TAG(tag)                                                                      \
  SVTKM_STATIC_ASSERT_MSG((::svtkm::detail::ListTagAssert<tag>::value),                              \
                         "Provided type is not a valid SVTK-m list tag.")

namespace internal
{

namespace detail
{

template <typename ListTag>
struct ListTagAsBrigandListImpl
{
  SVTKM_DEPRECATED_SUPPRESS_BEGIN
  SVTKM_IS_LIST_TAG(ListTag);
  using type = typename ListTag::list;
  SVTKM_DEPRECATED_SUPPRESS_END
};

} // namespace detail

/// Converts a ListTag to a brigand::list.
///
template <typename ListTag>
using ListTagAsBrigandList = typename detail::ListTagAsBrigandListImpl<ListTag>::type;

SVTKM_DEPRECATED_SUPPRESS_BEGIN
namespace detail
{

// Could use ListApply instead, but that causes deprecation warnings.
template <typename List>
struct ListAsListTagImpl;
template <typename... Ts>
struct ListAsListTagImpl<svtkm::List<Ts...>>
{
  using type = svtkm::ListTagBase<Ts...>;
};

} // namespace detail

template <typename List>
using ListAsListTag = typename detail::ListAsListTagImpl<List>::type;
SVTKM_DEPRECATED_SUPPRESS_END

// This allows the new `List` operations work on `ListTag`s.
template <typename T>
struct AsListImpl
{
  SVTKM_STATIC_ASSERT_MSG(ListTagCheck<T>::value,
                         "Attempted to use something that is not a List with a List operation.");
  SVTKM_DEPRECATED_SUPPRESS_BEGIN
  using type = typename std::conditional<std::is_base_of<svtkm::ListTagUniversal, T>::value,
                                         svtkm::ListUniversal,
                                         brigand::wrap<ListTagAsBrigandList<T>, svtkm::List>>::type;
  SVTKM_DEPRECATED_SUPPRESS_END
};

} // namespace internal


namespace detail
{

template <typename BrigandList, template <typename...> class Target>
struct ListTagApplyImpl;

template <typename... Ts, template <typename...> class Target>
struct ListTagApplyImpl<brigand::list<Ts...>, Target>
{
  using type = Target<Ts...>;
};

} // namespace detail

/// \brief Applies the list of types to a template.
///
/// Given a ListTag and a templated class, returns the class instantiated with the types
/// represented by the ListTag.
///
template <typename ListTag, template <typename...> class Target>
using ListTagApply SVTKM_DEPRECATED(1.6, "ListTagApply replaced by ListApply.") =
  typename detail::ListTagApplyImpl<internal::ListTagAsBrigandList<ListTag>, Target>::type;

/// A special tag for an empty list.
///
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagEmpty replaced by ListEmpty. Note that ListEmpty cannot be subclassed.") ListTagEmpty
  : detail::ListRoot
{
  using list = svtkm::detail::ListBase<>;
};

/// A tag that is a construction of two other tags joined together. This struct
/// can be subclassed and still behave like a list tag.
template <typename... ListTags>
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagJoin replaced by ListAppend. Note that ListAppend cannot be subclassed.") ListTagJoin
  : svtkm::internal::ListAsListTag<svtkm::ListAppend<ListTags...>>
{
};


/// A tag that is constructed by appending \c Type to \c ListTag.
template <typename ListTag, typename Type>
struct SVTKM_DEPRECATED(1.6,
                       "ListTagAppend<List, Type> replaced by ListAppend<List, svtkm::List<Type>. "
                       "Note that ListAppend cannot be subclassed.") ListTagAppend
  : svtkm::internal::ListAsListTag<svtkm::ListAppend<ListTag, svtkm::List<Type>>>
{
};

/// Append \c Type to \c ListTag only if \c ListTag does not already contain \c Type.
/// No checks are performed to see if \c ListTag itself has only unique elements.
template <typename ListTag, typename Type>
struct SVTKM_DEPRECATED(1.6) ListTagAppendUnique
  : std::conditional<
      svtkm::ListHas<ListTag, Type>::value,
      svtkm::internal::ListAsListTag<svtkm::internal::AsList<ListTag>>,
      svtkm::internal::ListAsListTag<svtkm::ListAppend<ListTag, svtkm::List<Type>>>>::type
{
};

/// A tag that consists of elements that are found in both tags. This struct
/// can be subclassed and still behave like a list tag.
template <typename ListTag1, typename ListTag2>
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagIntersect replaced by ListIntersect. Note that ListIntersect cannot be subclassed.")
  ListTagIntersect : svtkm::internal::ListAsListTag<svtkm::ListIntersect<ListTag1, ListTag2>>
{
};

/// A list tag that consists of each item in another list tag fed into a template that takes
/// a single parameter.
template <typename ListTag, template <typename> class Transform>
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagTransform replaced by ListTransform. Note that ListTransform cannot be subclassed.")
  ListTagTransform : svtkm::internal::ListAsListTag<svtkm::ListTransform<ListTag, Transform>>
{
};

/// A list tag that takes an existing ListTag and a predicate template that is applied to
/// each type in the ListTag. Any type in the ListTag that has a value element equal to true
/// (the equivalent of std::true_type), that item will be removed from the list. For example
/// the following type
///
/// ```cpp
/// svtkm::ListTagRemoveIf<svtkm::ListTagBase<int, float, long long, double>, std::is_integral>
/// ```
///
/// resolves to a ListTag that is equivalent to `svtkm::ListTag<float, double>` because
/// `std::is_integral<int>` and `std::is_integral<long long>` resolve to `std::true_type`
/// whereas `std::is_integral<float>` and `std::is_integral<double>` resolve to
/// `std::false_type`.
template <typename ListTag, template <typename> class Predicate>
struct SVTKM_DEPRECATED(
  1.6,
  "ListTagRemoveIf replaced by ListRemoveIf. Note that ListRemoveIf cannot be subclassed.")
  ListTagRemoveIf : svtkm::internal::ListAsListTag<svtkm::ListRemoveIf<ListTag, Predicate>>
{
};

namespace detail
{

// Old stlye ListCrossProduct expects brigand::list instead of svtkm::List. Transform back
template <typename List>
using ListToBrigand = svtkm::ListApply<List, brigand::list>;

} // namespace detail

/// Generate a tag that is the cross product of two other tags. The resulting
/// tag has the form of Tag< brigand::list<A1,B1>, brigand::list<A1,B2> .... >
///
template <typename ListTag1, typename ListTag2>
struct SVTKM_DEPRECATED(
  1.6,
  "ListCrossProduct replaced by ListCross. Note that LIstCross cannot be subclassed.")
  ListCrossProduct
  : svtkm::internal::ListAsListTag<
      svtkm::ListTransform<svtkm::ListCross<ListTag1, ListTag2>, detail::ListToBrigand>>
{
};

/// \brief Checks to see if the given \c Type is in the list pointed to by \c ListTag.
///
/// There is a static boolean named \c value that is set to true if the type is
/// contained in the list and false otherwise.
///
template <typename ListTag, typename Type>
struct SVTKM_DEPRECATED(1.6, "ListContains replaced by ListHas.") ListContains
  : svtkm::ListHas<ListTag, Type>
{
};

/// \brief Finds the type at the given index.
///
/// This struct contains subtype \c type that resolves to the type at the given index.
///
template <typename ListTag, svtkm::IdComponent Index>
struct SVTKM_DEPRECATED(1.6, "ListTypeAt::type replaced by ListAt.") ListTypeAt
{
  SVTKM_DEPRECATED_SUPPRESS_BEGIN
  SVTKM_IS_LIST_TAG(ListTag);
  SVTKM_DEPRECATED_SUPPRESS_END
  using type = brigand::at<internal::ListTagAsBrigandList<ListTag>,
                           std::integral_constant<svtkm::IdComponent, Index>>;
};

} // namespace svtkm

#endif //svtk_m_ListTag_h
