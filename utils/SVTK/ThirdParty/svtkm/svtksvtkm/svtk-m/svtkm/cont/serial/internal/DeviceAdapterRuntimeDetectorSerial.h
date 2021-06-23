//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_serial_internal_DeviceAdapterRuntimeDetector_h
#define svtk_m_cont_serial_internal_DeviceAdapterRuntimeDetector_h

#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;

/// Determine if this machine supports Serial backend
///
template <>
class SVTKM_CONT_EXPORT DeviceAdapterRuntimeDetector<svtkm::cont::DeviceAdapterTagSerial>
{
public:
  /// Returns true if the given device adapter is supported on the current
  /// machine.
  SVTKM_CONT bool Exists() const;
};
}
}

#endif
