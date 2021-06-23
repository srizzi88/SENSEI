//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ErrorBadDevice.h>

#include <string>

namespace svtkm
{
namespace cont
{

void throwFailedRuntimeDeviceTransfer(const std::string& className,
                                      svtkm::cont::DeviceAdapterId deviceId)
{ //Should we support typeid() instead of className?
  const std::string msg = "SVTK-m was unable to transfer " + className + " to DeviceAdapter[id=" +
    std::to_string(deviceId.GetValue()) + ", name=" + deviceId.GetName() +
    "]. This is generally caused by asking for execution on a DeviceAdapter that "
    "isn't compiled into SVTK-m. In the case of CUDA it can also be caused by accidentally "
    "compiling source files as C++ files instead of CUDA.";
  throw svtkm::cont::ErrorBadDevice(msg);
}
}
}
