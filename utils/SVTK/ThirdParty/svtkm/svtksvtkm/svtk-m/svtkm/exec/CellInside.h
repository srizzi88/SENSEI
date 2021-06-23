//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_CellInside_h
#define svtk_m_exec_CellInside_h

#include <svtkm/CellShape.h>
#include <svtkm/Types.h>

#include <lcl/lcl.h>

namespace svtkm
{
namespace exec
{

template <typename T, typename CellShapeTag>
static inline SVTKM_EXEC bool CellInside(const svtkm::Vec<T, 3>& pcoords, CellShapeTag)
{
  using VtkcTagType = typename svtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type;
  return lcl::cellInside(VtkcTagType{}, pcoords);
}

template <typename T>
static inline SVTKM_EXEC bool CellInside(const svtkm::Vec<T, 3>&, svtkm::CellShapeTagEmpty)
{
  return false;
}

template <typename T>
static inline SVTKM_EXEC bool CellInside(const svtkm::Vec<T, 3>& pcoords, svtkm::CellShapeTagPolyLine)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1);
}

/// Checks if the parametric coordinates `pcoords` are on the inside for the
/// specified cell type.
///
template <typename T>
static inline SVTKM_EXEC bool CellInside(const svtkm::Vec<T, 3>& pcoords,
                                        svtkm::CellShapeTagGeneric shape)
{
  bool result = false;
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(result = CellInside(pcoords, CellShapeTag()));
    default:
      break;
  }

  return result;
}
}
} // svtkm::exec

#endif // svtk_m_exec_CellInside_h
