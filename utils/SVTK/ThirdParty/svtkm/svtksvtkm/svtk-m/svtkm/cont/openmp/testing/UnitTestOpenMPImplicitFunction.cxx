//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <svtkm/cont/testing/TestingImplicitFunction.h>

namespace
{

void TestImplicitFunctions()
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagOpenMP{});

  svtkm::cont::testing::TestingImplicitFunction testing;
  testing.Run(svtkm::cont::DeviceAdapterTagOpenMP());
}

} // anonymous namespace

int UnitTestOpenMPImplicitFunction(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestImplicitFunctions, argc, argv);
}
