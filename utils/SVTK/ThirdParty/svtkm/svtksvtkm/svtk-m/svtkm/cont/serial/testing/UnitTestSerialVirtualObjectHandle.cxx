//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/testing/TestingVirtualObjectHandle.h>

namespace
{

void TestVirtualObjectHandle()
{
  using DeviceAdapterList = svtkm::List<svtkm::cont::DeviceAdapterTagSerial>;
  svtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList>::Run();
}

} // anonymous namespace

int UnitTestSerialVirtualObjectHandle(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::Testing::Run(TestVirtualObjectHandle, argc, argv);
}
