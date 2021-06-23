//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtkm_cont_ArrayHandle_cxx
#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace cont
{

#define SVTKM_ARRAYHANDLE_INSTANTIATE(Type)                                                         \
  template class SVTKM_CONT_EXPORT ArrayHandle<Type, StorageTagBasic>;                              \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 2>, StorageTagBasic>;                \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 3>, StorageTagBasic>;                \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 4>, StorageTagBasic>;

SVTKM_ARRAYHANDLE_INSTANTIATE(char)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Int8)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::UInt8)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Int16)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::UInt16)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Int32)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::UInt32)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Int64)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::UInt64)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Float32)
SVTKM_ARRAYHANDLE_INSTANTIATE(svtkm::Float64)

#undef SVTKM_ARRAYHANDLE_INSTANTIATE
}
} // end svtkm::cont
