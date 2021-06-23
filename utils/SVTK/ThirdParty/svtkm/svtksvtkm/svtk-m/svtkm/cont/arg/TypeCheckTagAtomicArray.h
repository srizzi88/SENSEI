//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TypeCheckTagAtomicArray_h
#define svtk_m_cont_arg_TypeCheckTagAtomicArray_h

#include <svtkm/cont/arg/TypeCheck.h>

#include <svtkm/List.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageBasic.h>
#include <svtkm/cont/StorageVirtual.h>

#include <svtkm/cont/AtomicArray.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// The atomic array type check passes for an \c ArrayHandle of a structure
/// that is valid for atomic access. There are many restrictions on the
/// type of data that can be used for an atomic array.
///
struct TypeCheckTagAtomicArray;

template <typename ArrayType>
struct TypeCheck<TypeCheckTagAtomicArray, ArrayType>
{
  static constexpr bool value = false;
};

template <typename T>
struct TypeCheck<TypeCheckTagAtomicArray, svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>>
{
  static constexpr bool value = svtkm::ListHas<svtkm::cont::AtomicArrayTypeList, T>::value;
};

template <typename T>
struct TypeCheck<TypeCheckTagAtomicArray, svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>>
{
  static constexpr bool value = svtkm::ListHas<svtkm::cont::AtomicArrayTypeList, T>::value;
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TypeCheckTagAtomicArray_h
