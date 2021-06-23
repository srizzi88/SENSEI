//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

// cuda portals created from basic array handles should produce raw device
// pointers with ArrayPortalToIterator (see ArrayPortalFromThrust).
void TestIteratorSpecialization()
{
  svtkm::cont::ArrayHandle<int> handle;

  auto outputPortal = handle.PrepareForOutput(1, svtkm::cont::DeviceAdapterTagCuda{});
  auto inputPortal = handle.PrepareForInput(svtkm::cont::DeviceAdapterTagCuda{});
  auto inPlacePortal = handle.PrepareForInPlace(svtkm::cont::DeviceAdapterTagCuda{});

  auto outputIter = svtkm::cont::ArrayPortalToIteratorBegin(outputPortal);
  auto inputIter = svtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
  auto inPlaceIter = svtkm::cont::ArrayPortalToIteratorBegin(inPlacePortal);

  (void)outputIter;
  (void)inputIter;
  (void)inPlaceIter;

  SVTKM_TEST_ASSERT(std::is_same<decltype(outputIter), int*>::value);
  SVTKM_TEST_ASSERT(std::is_same<decltype(inputIter), int const*>::value);
  SVTKM_TEST_ASSERT(std::is_same<decltype(inPlaceIter), int*>::value);
}

} // end anon namespace

int UnitTestCudaIterators(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagCuda{});
  return svtkm::cont::testing::Testing::Run(TestIteratorSpecialization, argc, argv);
}
