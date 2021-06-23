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

#include <svtkm/cont/ErrorBadValue.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace svtkm
{
namespace cont
{

namespace detail
{

struct RuntimeDeviceTrackerInternals
{
  bool RuntimeAllowed[SVTKM_MAX_DEVICE_ADAPTER_ID];
};
}

SVTKM_CONT
RuntimeDeviceTracker::RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details,
                                           bool reset)
  : Internals(details)
{
  if (reset)
  {
    this->Reset();
  }
}

SVTKM_CONT
RuntimeDeviceTracker::~RuntimeDeviceTracker()
{
}

SVTKM_CONT
void RuntimeDeviceTracker::CheckDevice(svtkm::cont::DeviceAdapterId deviceId) const
{
  if (!deviceId.IsValueValid())
  {
    std::stringstream message;
    message << "Device '" << deviceId.GetName() << "' has invalid ID of "
            << (int)deviceId.GetValue();
    throw svtkm::cont::ErrorBadValue(message.str());
  }
}

SVTKM_CONT
bool RuntimeDeviceTracker::CanRunOn(svtkm::cont::DeviceAdapterId deviceId) const
{
  if (deviceId == svtkm::cont::DeviceAdapterTagAny{})
  { //If at least a single device is enabled, than any device is enabled
    for (svtkm::Int8 i = 1; i < SVTKM_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      if (this->Internals->RuntimeAllowed[static_cast<std::size_t>(i)])
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    this->CheckDevice(deviceId);
    return this->Internals->RuntimeAllowed[deviceId.GetValue()];
  }
}

SVTKM_CONT
void RuntimeDeviceTracker::SetDeviceState(svtkm::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);

  this->Internals->RuntimeAllowed[deviceId.GetValue()] = state;
}


SVTKM_CONT void RuntimeDeviceTracker::ResetDevice(svtkm::cont::DeviceAdapterId deviceId)
{
  if (deviceId == svtkm::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    svtkm::cont::RuntimeDeviceInformation runtimeDevice;
    this->SetDeviceState(deviceId, runtimeDevice.Exists(deviceId));
    this->LogEnabledDevices();
  }
}


SVTKM_CONT
void RuntimeDeviceTracker::Reset()
{
  std::fill_n(this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, false);

  // We use this instead of calling CheckDevice/SetDeviceState so that
  // when we use logging we get better messages stating we are reseting
  // the devices.
  svtkm::cont::RuntimeDeviceInformation runtimeDevice;
  for (svtkm::Int8 i = 1; i < SVTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    svtkm::cont::DeviceAdapterId device = svtkm::cont::make_DeviceAdapterId(i);
    if (device.IsValueValid())
    {
      const bool state = runtimeDevice.Exists(device);
      this->Internals->RuntimeAllowed[device.GetValue()] = state;
    }
  }
  this->LogEnabledDevices();
}

SVTKM_CONT void RuntimeDeviceTracker::DisableDevice(svtkm::cont::DeviceAdapterId deviceId)
{
  if (deviceId == svtkm::cont::DeviceAdapterTagAny{})
  {
    std::fill_n(this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, false);
  }
  else
  {
    this->SetDeviceState(deviceId, false);
  }
  this->LogEnabledDevices();
}

SVTKM_CONT
void RuntimeDeviceTracker::ForceDevice(DeviceAdapterId deviceId)
{
  if (deviceId == svtkm::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    this->CheckDevice(deviceId);
    svtkm::cont::RuntimeDeviceInformation runtimeDevice;
    const bool runtimeExists = runtimeDevice.Exists(deviceId);
    if (!runtimeExists)
    {
      std::stringstream message;
      message << "Cannot force to device '" << deviceId.GetName()
              << "' because that device is not available on this system";
      throw svtkm::cont::ErrorBadValue(message.str());
    }

    std::fill_n(this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, false);

    this->Internals->RuntimeAllowed[deviceId.GetValue()] = runtimeExists;
    this->LogEnabledDevices();
  }
}

SVTKM_CONT
void RuntimeDeviceTracker::PrintSummary(std::ostream& out) const
{
  for (svtkm::Int8 i = 1; i < SVTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto dev = svtkm::cont::make_DeviceAdapterId(i);
    out << " - Device " << static_cast<svtkm::Int32>(i) << " (" << dev.GetName()
        << "): Enabled=" << this->CanRunOn(dev) << "\n";
  }
}

SVTKM_CONT
void RuntimeDeviceTracker::LogEnabledDevices() const
{
  std::stringstream message;
  message << "Enabled devices:";
  bool atLeastOneDeviceEnabled = false;
  for (svtkm::Int8 deviceIndex = 1; deviceIndex < SVTKM_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    svtkm::cont::DeviceAdapterId device = svtkm::cont::make_DeviceAdapterId(deviceIndex);
    if (this->CanRunOn(device))
    {
      message << " " << device.GetName();
      atLeastOneDeviceEnabled = true;
    }
  }
  if (!atLeastOneDeviceEnabled)
  {
    message << " NONE!";
  }
  SVTKM_LOG_S(svtkm::cont::LogLevel::DevicesEnabled, message.str());
}

SVTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(svtkm::cont::DeviceAdapterId device,
                                                       RuntimeDeviceTrackerMode mode)
  : RuntimeDeviceTracker(GetRuntimeDeviceTracker().Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);

  if (mode == RuntimeDeviceTrackerMode::Force)
  {
    this->ForceDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Enable)
  {
    this->ResetDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Disable)
  {
    this->DisableDevice(device);
  }
}

SVTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  svtkm::cont::DeviceAdapterId device,
  RuntimeDeviceTrackerMode mode,
  const svtkm::cont::RuntimeDeviceTracker& tracker)
  : RuntimeDeviceTracker(tracker.Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);
  if (mode == RuntimeDeviceTrackerMode::Force)
  {
    this->ForceDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Enable)
  {
    this->ResetDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Disable)
  {
    this->DisableDevice(device);
  }
}

SVTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  const svtkm::cont::RuntimeDeviceTracker& tracker)
  : RuntimeDeviceTracker(tracker.Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);
}

SVTKM_CONT
ScopedRuntimeDeviceTracker::~ScopedRuntimeDeviceTracker()
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::DevicesEnabled, "Leaving scoped runtime region");
  std::copy_n(
    this->SavedState->RuntimeAllowed, SVTKM_MAX_DEVICE_ADAPTER_ID, this->Internals->RuntimeAllowed);
  this->LogEnabledDevices();
}

SVTKM_CONT
svtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker()
{
#if defined(SVTKM_CLANG) && defined(__apple_build_version__) && (__apple_build_version__ < 8000000)
  static std::mutex mtx;
  static std::map<std::thread::id, svtkm::cont::RuntimeDeviceTracker*> globalTrackers;
  static std::map<std::thread::id, svtkm::cont::detail::RuntimeDeviceTrackerInternals*>
    globalTrackerInternals;
  std::thread::id this_id = std::this_thread::get_id();

  std::unique_lock<std::mutex> lock(mtx);
  auto iter = globalTrackers.find(this_id);
  if (iter != globalTrackers.end())
  {
    return *iter->second;
  }
  else
  {
    auto* details = new svtkm::cont::detail::RuntimeDeviceTrackerInternals();
    svtkm::cont::RuntimeDeviceTracker* tracker = new svtkm::cont::RuntimeDeviceTracker(details, true);
    globalTrackers[this_id] = tracker;
    globalTrackerInternals[this_id] = details;
    return *tracker;
  }
#else
  static thread_local svtkm::cont::detail::RuntimeDeviceTrackerInternals details;
  static thread_local svtkm::cont::RuntimeDeviceTracker runtimeDeviceTracker(&details, true);
  return runtimeDeviceTracker;
#endif
}
}
} // namespace svtkm::cont
