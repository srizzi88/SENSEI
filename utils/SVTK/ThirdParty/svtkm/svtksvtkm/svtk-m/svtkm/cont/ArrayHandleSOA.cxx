//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtkm_cont_ArrayHandleSOA_cxx
#include <svtkm/cont/ArrayHandleSOA.h>

namespace svtkm
{
namespace cont
{

#define SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(Type)                                                     \
  template class SVTKM_CONT_EXPORT ArrayHandle<Type, StorageTagSOA>;                                \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 2>, StorageTagSOA>;                  \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 3>, StorageTagSOA>;                  \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<Type, 4>, StorageTagSOA>;

SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(char)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Int8)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::UInt8)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Int16)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::UInt16)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Int32)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::UInt32)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Int64)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::UInt64)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Float32)
SVTKM_ARRAYHANDLE_SOA_INSTANTIATE(svtkm::Float64)

#undef SVTKM_ARRAYHANDLE_SOA_INSTANTIATE
}
} // end svtkm::cont
