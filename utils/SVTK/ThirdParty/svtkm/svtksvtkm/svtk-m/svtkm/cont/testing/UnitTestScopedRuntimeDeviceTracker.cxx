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

//include all backends
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <array>

namespace
{

template <typename DeviceAdapterTag>
void verify_state(DeviceAdapterTag tag, std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID>& defaults)
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  // presumable all other devices match the defaults
  for (svtkm::Int8 i = 1; i < SVTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    const auto deviceId = svtkm::cont::make_DeviceAdapterId(i);
    if (deviceId != tag)
    {
      SVTKM_TEST_ASSERT(defaults[static_cast<std::size_t>(i)] == tracker.CanRunOn(deviceId),
                       "ScopedRuntimeDeviceTracker didn't properly setup state correctly");
    }
  }
}

template <typename DeviceAdapterTag>
void verify_srdt_support(DeviceAdapterTag tag,
                         std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID>& force,
                         std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID>& enable,
                         std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID>& disable)
{
  svtkm::cont::RuntimeDeviceInformation runtime;
  const bool haveSupport = runtime.Exists(tag);
  if (haveSupport)
  {
    svtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   svtkm::cont::RuntimeDeviceTrackerMode::Force);
    SVTKM_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport, "");
    verify_state(tag, force);
  }

  if (haveSupport)
  {
    svtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   svtkm::cont::RuntimeDeviceTrackerMode::Enable);
    SVTKM_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport);
    verify_state(tag, enable);
  }

  {
    svtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   svtkm::cont::RuntimeDeviceTrackerMode::Disable);
    SVTKM_TEST_ASSERT(tracker.CanRunOn(tag) == false, "");
    verify_state(tag, disable);
  }
}

void VerifyScopedRuntimeDeviceTracker()
{
  std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID> all_off;
  std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID> all_on;
  std::array<bool, SVTKM_MAX_DEVICE_ADAPTER_ID> defaults;

  all_off.fill(false);
  svtkm::cont::RuntimeDeviceInformation runtime;
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  for (svtkm::Int8 i = 1; i < SVTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto deviceId = svtkm::cont::make_DeviceAdapterId(i);
    defaults[static_cast<std::size_t>(i)] = tracker.CanRunOn(deviceId);
    all_on[static_cast<std::size_t>(i)] = runtime.Exists(deviceId);
  }

  using SerialTag = ::svtkm::cont::DeviceAdapterTagSerial;
  using OpenMPTag = ::svtkm::cont::DeviceAdapterTagOpenMP;
  using TBBTag = ::svtkm::cont::DeviceAdapterTagTBB;
  using CudaTag = ::svtkm::cont::DeviceAdapterTagCuda;
  using AnyTag = ::svtkm::cont::DeviceAdapterTagAny;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  verify_srdt_support(SerialTag(), all_off, all_on, defaults);
  verify_srdt_support(OpenMPTag(), all_off, all_on, defaults);
  verify_srdt_support(CudaTag(), all_off, all_on, defaults);
  verify_srdt_support(TBBTag(), all_off, all_on, defaults);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);


  verify_srdt_support(AnyTag(), all_on, all_on, all_off);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);
}

} // anonymous namespace

int UnitTestScopedRuntimeDeviceTracker(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(VerifyScopedRuntimeDeviceTracker, argc, argv);
}
