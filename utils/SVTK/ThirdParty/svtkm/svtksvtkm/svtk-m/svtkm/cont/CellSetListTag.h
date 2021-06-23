//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetListTag_h
#define svtk_m_cont_CellSetListTag_h

// Everything in this header file is deprecated and movded to CellSetList.h.

#ifndef SVTKM_DEFAULT_CELL_SET_LIST_TAG
#define SVTKM_DEFAULT_CELL_SET_LIST_TAG ::svtkm::cont::detail::CellSetListTagDefault
#endif

#include <svtkm/ListTag.h>

#include <svtkm/cont/CellSetList.h>

#define SVTK_M_OLD_CELL_LIST_DEFINITION(name)                                                       \
  struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(                                                       \
    1.6,                                                                                           \
    "CellSetListTag" #name " replaced by CellSetList" #name ". "                                   \
    "Note that the new CellSetList" #name " cannot be subclassed.") CellSetListTag##name           \
    : svtkm::internal::ListAsListTag<CellSetList##name>                                             \
  {                                                                                                \
  }

namespace svtkm
{
namespace cont
{

SVTK_M_OLD_CELL_LIST_DEFINITION(Structured1D);
SVTK_M_OLD_CELL_LIST_DEFINITION(Structured2D);
SVTK_M_OLD_CELL_LIST_DEFINITION(Structured3D);
SVTK_M_OLD_CELL_LIST_DEFINITION(ExplicitDefault);
SVTK_M_OLD_CELL_LIST_DEFINITION(Common);
SVTK_M_OLD_CELL_LIST_DEFINITION(Structured);
SVTK_M_OLD_CELL_LIST_DEFINITION(Unstructured);

template <typename ShapesStorageTag = SVTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = SVTKM_DEFAULT_OFFSETS_STORAGE_TAG>
struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(
  1.6,
  "CellSetListTagExplicit replaced by CellSetListExplicit. "
  "Note that the new CellSetListExplicit cannot be subclassed.") CellSetListTagExplicit
  : svtkm::internal::ListAsListTag<
      CellSetListExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>>
{
};

namespace detail
{

struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(
  1.6,
  "SVTKM_DEFAULT_CELL_SET_LIST_TAG replaced by SVTKM_DEFAULT_CELL_SET_LIST. "
  "Note that the new SVTKM_DEFAULT_CELL_SET_LIST cannot be subclassed.") CellSetListTagDefault
  : svtkm::internal::ListAsListTag<SVTKM_DEFAULT_CELL_SET_LIST>
{
};

} // namespace detail
}
} // namespace svtkm::cont

#undef SVTK_M_OLD_CELL_LIST_DEFINITION

#endif //svtk_m_cont_CellSetListTag_h
