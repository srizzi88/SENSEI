//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetList_h
#define svtk_m_cont_CellSetList_h

#ifndef SVTKM_DEFAULT_CELL_SET_LIST
#define SVTKM_DEFAULT_CELL_SET_LIST ::svtkm::cont::CellSetListCommon
#endif

#include <svtkm/List.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetExtrude.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>

namespace svtkm
{
namespace cont
{

using CellSetListStructured1D = svtkm::List<svtkm::cont::CellSetStructured<1>>;

using CellSetListStructured2D = svtkm::List<svtkm::cont::CellSetStructured<2>>;

using CellSetListStructured3D = svtkm::List<svtkm::cont::CellSetStructured<3>>;


template <typename ShapesStorageTag = SVTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = SVTKM_DEFAULT_OFFSETS_STORAGE_TAG>
using CellSetListExplicit = svtkm::List<
  svtkm::cont::CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>>;

using CellSetListExplicitDefault = CellSetListExplicit<>;

using CellSetListCommon = svtkm::List<svtkm::cont::CellSetStructured<2>,
                                     svtkm::cont::CellSetStructured<3>,
                                     svtkm::cont::CellSetExplicit<>,
                                     svtkm::cont::CellSetSingleType<>>;

using CellSetListStructured =
  svtkm::List<svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3>>;

using CellSetListUnstructured =
  svtkm::List<svtkm::cont::CellSetExplicit<>, svtkm::cont::CellSetSingleType<>>;
}
} // namespace svtkm::cont

#endif //svtk_m_cont_CellSetList_h
