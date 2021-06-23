//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_serial_internal_AtomicInterfaceExecutionSerial_h
#define svtk_m_cont_serial_internal_AtomicInterfaceExecutionSerial_h

#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <svtkm/cont/internal/AtomicInterfaceControl.h>
#include <svtkm/cont/internal/AtomicInterfaceExecution.h>

#include <svtkm/Types.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <>
class AtomicInterfaceExecution<DeviceAdapterTagSerial> : public AtomicInterfaceControl
{
};
}
}
} // end namespace svtkm::cont::internal

#endif // svtk_m_cont_serial_internal_AtomicInterfaceExecutionSerial_h
