//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagBitField_h
#define svtk_m_cont_arg_TypeCheckTagBitField_h

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/cont/BitField.h>

#include <type_traits>

namespace svtkm
{
namespace cont
{
namespace arg
{

struct TypeCheckTagBitField
{
};

template <typename T>
struct TypeCheck<TypeCheckTagBitField, T> : public std::is_base_of<svtkm::cont::BitField, T>
{
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagBitField_h
