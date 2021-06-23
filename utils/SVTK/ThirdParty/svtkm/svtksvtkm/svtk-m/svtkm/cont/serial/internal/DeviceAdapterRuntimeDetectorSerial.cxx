//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>

namespace svtkm
{
namespace cont
{
SVTKM_CONT bool DeviceAdapterRuntimeDetector<svtkm::cont::DeviceAdapterTagSerial>::Exists() const
{
  return svtkm::cont::DeviceAdapterTagSerial::IsEnabled;
}
}
}
