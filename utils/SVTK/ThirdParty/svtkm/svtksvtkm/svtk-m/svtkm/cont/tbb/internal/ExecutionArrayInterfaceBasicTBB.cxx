//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/tbb/internal/ExecutionArrayInterfaceBasicTBB.h>

namespace svtkm
{
namespace cont
{
namespace internal
{
DeviceAdapterId ExecutionArrayInterfaceBasic<DeviceAdapterTagTBB>::GetDeviceId() const
{
  return DeviceAdapterTagTBB{};
}

} // namespace internal
}
} // namespace svtkm::cont
