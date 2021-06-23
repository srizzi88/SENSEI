//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_serial_internal_VirtualObjectTransferSerial_h
#define svtk_m_cont_serial_internal_VirtualObjectTransferSerial_h

#include <svtkm/cont/internal/VirtualObjectTransfer.h>
#include <svtkm/cont/internal/VirtualObjectTransferShareWithControl.h>
#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualDerivedType>
struct VirtualObjectTransfer<VirtualDerivedType, svtkm::cont::DeviceAdapterTagSerial> final
  : VirtualObjectTransferShareWithControl<VirtualDerivedType>
{
  using VirtualObjectTransferShareWithControl<
    VirtualDerivedType>::VirtualObjectTransferShareWithControl;
};
}
}
} // svtkm::cont::internal

#endif // svtk_m_cont_serial_internal_VirtualObjectTransferSerial_h
