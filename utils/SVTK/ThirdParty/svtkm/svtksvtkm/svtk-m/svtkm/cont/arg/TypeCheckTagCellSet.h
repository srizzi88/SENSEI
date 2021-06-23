//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagCellSet_h
#define svtk_m_cont_arg_TypeCheckTagCellSet_h

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/cont/CellSet.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// Check for a CellSet-like object.
///
struct TypeCheckTagCellSet
{
};

template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSet, CellSetType>
{
  static constexpr bool value = svtkm::cont::internal::CellSetCheck<CellSetType>::type::value;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagCellSet_h
