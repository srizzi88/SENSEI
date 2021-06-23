//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagCellSetStructured_h
#define svtk_m_cont_arg_TypeCheckTagCellSetStructured_h

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/cont/CellSet.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// Check for a Structured CellSet-like object.
///
struct TypeCheckTagCellSetStructured
{
};


template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSetStructured, CellSetType>
{
  using is_3d_cellset = std::is_same<CellSetType, svtkm::cont::CellSetStructured<3>>;
  using is_2d_cellset = std::is_same<CellSetType, svtkm::cont::CellSetStructured<2>>;
  using is_1d_cellset = std::is_same<CellSetType, svtkm::cont::CellSetStructured<1>>;

  static constexpr bool value =
    is_3d_cellset::value || is_2d_cellset::value || is_1d_cellset::value;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagCellSetStructured_h
