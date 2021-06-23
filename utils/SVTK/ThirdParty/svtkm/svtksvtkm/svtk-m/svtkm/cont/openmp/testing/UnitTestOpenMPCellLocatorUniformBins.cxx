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
#include <svtkm/cont/testing/TestingCellLocatorUniformBins.h>

int UnitTestOpenMPCellLocatorUniformBins(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagOpenMP{});
  return svtkm::cont::testing::Testing::Run(
    TestingCellLocatorUniformBins<svtkm::cont::DeviceAdapterTagOpenMP>, argc, argv);
}
