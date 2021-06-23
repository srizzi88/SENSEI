//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtk_m_worklet_Keys_cxx
#include <svtkm/worklet/Keys.h>
#include <svtkm/worklet/Keys.hxx>

#define SVTK_M_KEYS_EXPORT(T)                                                                       \
  template class SVTKM_WORKLET_EXPORT svtkm::worklet::Keys<T>;                                       \
  template SVTKM_WORKLET_EXPORT SVTKM_CONT void svtkm::worklet::Keys<T>::BuildArrays(                 \
    const svtkm::cont::ArrayHandle<T>& keys,                                                        \
    svtkm::worklet::KeysSortType sort,                                                              \
    svtkm::cont::DeviceAdapterId device);                                                           \
  template SVTKM_WORKLET_EXPORT SVTKM_CONT void svtkm::worklet::Keys<T>::BuildArrays(                 \
    const svtkm::cont::ArrayHandleVirtual<T>& keys,                                                 \
    svtkm::worklet::KeysSortType sort,                                                              \
    svtkm::cont::DeviceAdapterId device)

SVTK_M_KEYS_EXPORT(svtkm::HashType);
SVTK_M_KEYS_EXPORT(svtkm::UInt8);
using Pair_UInt8_Id2 = svtkm::Pair<svtkm::UInt8, svtkm::Id2>;
SVTK_M_KEYS_EXPORT(Pair_UInt8_Id2);


#undef SVTK_M_KEYS_EXPORT
