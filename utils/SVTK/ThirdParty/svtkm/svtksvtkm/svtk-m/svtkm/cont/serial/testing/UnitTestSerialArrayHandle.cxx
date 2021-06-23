//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/testing/TestingArrayHandles.h>

int UnitTestSerialArrayHandle(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::TestingArrayHandles<svtkm::cont::DeviceAdapterTagSerial>::Run(argc,
                                                                                           argv);
}
