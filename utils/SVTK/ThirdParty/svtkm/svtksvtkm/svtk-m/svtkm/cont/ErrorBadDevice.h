//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ErrorBadDevice_h
#define svtk_m_cont_ErrorBadDevice_h

#include <svtkm/cont/Error.h>

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace cont
{


SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is thrown when SVTK-m performs an operation that is not supported
/// on the current device.
///
class SVTKM_ALWAYS_EXPORT ErrorBadDevice : public Error
{
public:
  ErrorBadDevice(const std::string& message)
    : Error(message)
  {
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END

/// Throws an ErrorBadeDevice exception with the following message:
/// "SVTK-m was unable to transfer \c className to DeviceAdapter[id,name].
///  This is generally caused by asking for execution on a DeviceAdapter that
///  isn't compiled into SVTK-m. In the case of CUDA it can also be caused by accidentally
///  compiling source files as C++ files instead of CUDA."
//
SVTKM_CONT_EXPORT void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                                       svtkm::cont::DeviceAdapterId device);
}
} // namespace svtkm::cont

#endif // svtk_m_cont_ErrorBadDevice_h
