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
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();

  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagTBB{});
  using DeviceAdapterList = svtkm::List<svtkm::cont::DeviceAdapterTagTBB>;
  svtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList>::Run();

  tracker.Reset();
  using DeviceAdapterList2 =
    svtkm::List<svtkm::cont::DeviceAdapterTagSerial, svtkm::cont::DeviceAdapterTagTBB>;
  svtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList2>::Run();
}


} // anonymous namespace

int UnitTestTBBVirtualObjectHandle(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestVirtualObjectHandle, argc, argv);
}
