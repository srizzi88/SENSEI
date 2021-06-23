//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtk_m_cont_serial_internal_ArrayManagerExecutionSerial_cxx

#include <svtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>

namespace svtkm
{
namespace cont
{

SVTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagSerial)
}
} // end svtkm::cont
