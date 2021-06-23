//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtk_m_cont_ArrayHandleVirtual_cxx
#include <svtkm/cont/ArrayHandleVirtual.h>

namespace svtkm
{
namespace cont
{

#define SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(T)                                                  \
  template class SVTKM_CONT_EXPORT ArrayHandle<T, StorageTagVirtual>;                               \
  template class SVTKM_CONT_EXPORT ArrayHandleVirtual<T>;                                           \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<T, 2>, StorageTagVirtual>;                 \
  template class SVTKM_CONT_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 2>>;                             \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<T, 3>, StorageTagVirtual>;                 \
  template class SVTKM_CONT_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 3>>;                             \
  template class SVTKM_CONT_EXPORT ArrayHandle<svtkm::Vec<T, 4>, StorageTagVirtual>;                 \
  template class SVTKM_CONT_EXPORT ArrayHandleVirtual<svtkm::Vec<T, 4>>

SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(char);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Int8);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::UInt8);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Int16);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::UInt16);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Int32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::UInt32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Int64);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::UInt64);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Float32);
SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE(svtkm::Float64);

#undef SVTK_M_ARRAY_HANDLE_VIRTUAL_INSTANTIATE
}
} //namespace svtkm::cont
