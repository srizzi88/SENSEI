//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>
#include <svtkm/cont/testing/TestingBitField.h>

int UnitTestTBBBitField(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagTBB{});
  return svtkm::cont::testing::TestingBitField<svtkm::cont::DeviceAdapterTagTBB>::Run(argc, argv);
}
