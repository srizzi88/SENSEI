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
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/testing/TestingGeometry.h>

int UnitTestCudaGeometry(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagCuda{});
  return svtkm::cont::testing::Testing::Run(
    UnitTestGeometryNamespace::RunGeometryTests<svtkm::cont::DeviceAdapterTagCuda>, argc, argv);
}
