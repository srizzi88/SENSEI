//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_TypeListTag_h
#define svtk_m_TypeListTag_h

// Everything in this header file is deprecated and movded to TypeList.h.

#ifndef SVTKM_DEFAULT_TYPE_LIST_TAG
#define SVTKM_DEFAULT_TYPE_LIST_TAG ::svtkm::internal::TypeListTagDefault
#endif

#include <svtkm/ListTag.h>
#include <svtkm/TypeList.h>

#define SVTK_M_OLD_TYPE_LIST_DEFINITION(name) \
  struct SVTKM_ALWAYS_EXPORT \
  SVTKM_DEPRECATED( \
    1.6, \
    "TypeListTag" #name " replaced by TypeList" #name ". " \
    "Note that the new TypeList" #name " cannot be subclassed.") \
  TypeListTag ## name : svtkm::internal::ListAsListTag<TypeList ## name> \
  { \
  }

SVTKM_DEPRECATED_SUPPRESS_BEGIN

namespace svtkm
{

SVTK_M_OLD_TYPE_LIST_DEFINITION(Id);
SVTK_M_OLD_TYPE_LIST_DEFINITION(Id2);
SVTK_M_OLD_TYPE_LIST_DEFINITION(Id3);
SVTK_M_OLD_TYPE_LIST_DEFINITION(IdComponent);
SVTK_M_OLD_TYPE_LIST_DEFINITION(Index);
SVTK_M_OLD_TYPE_LIST_DEFINITION(FieldScalar);
SVTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec2);
SVTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec3);
SVTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec4);
SVTK_M_OLD_TYPE_LIST_DEFINITION(FloatVec);
SVTK_M_OLD_TYPE_LIST_DEFINITION(Field);
SVTK_M_OLD_TYPE_LIST_DEFINITION(ScalarAll);
SVTK_M_OLD_TYPE_LIST_DEFINITION(VecCommon);
SVTK_M_OLD_TYPE_LIST_DEFINITION(VecAll);
SVTK_M_OLD_TYPE_LIST_DEFINITION(All);
SVTK_M_OLD_TYPE_LIST_DEFINITION(Common);

namespace internal
{

SVTK_M_OLD_TYPE_LIST_DEFINITION(VecUncommon);

// Special definition of TypeListTagCommon to give descriptive warning when
// SVTKM_DEFAULT_TYPE_LIST_TAG is used.
struct SVTKM_ALWAYS_EXPORT
SVTKM_DEPRECATED(
  1.6,
  "SVTKM_DEFAULT_TYPE_LIST_TAG replaced by SVTKM_DEFAULT_TYPE_LIST. "
  "Note that the new SVTKM_DEFAULT_TYPE_LIST cannot be subclassed.")
TypeListTagDefault : svtkm::internal::ListAsListTag<SVTKM_DEFAULT_TYPE_LIST>
{
};

} // namespace internal

// Special implementation of ListContains for TypeListTagAll to always be
// true. Although TypeListTagAll is necessarily finite, the point is to
// be all inclusive. Besides, this should speed up the compilation when
// checking a list that should contain everything.
template<typename Type>
struct ListContains<svtkm::TypeListTagAll, Type>
{
  static constexpr bool value = true;
};

// Special implementation of ListHas for TypeListTagAll to always be
// true. Although TypeListTagAll is necessarily finite, the point is to
// be all inclusive. Besides, this should speed up the compilation when
// checking a list that should contain everything.
namespace detail
{

template<typename Type>
struct ListHasImpl<svtkm::TypeListTagAll, Type>
{
  using type = std::true_type;
};

} // namespace detail

} // namespace svtkm

SVTKM_DEPRECATED_SUPPRESS_END
#undef SVTK_M_OLD_TYPE_LIST_DEFINITION

#endif //svtk_m_TypeListTag_h
