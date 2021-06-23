//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Timer_h
#define svtk_m_cont_Timer_h

#include <svtkm/List.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterList.h>

#include <svtkm/cont/svtkm_cont_export.h>

#include <memory>

namespace svtkm
{
namespace cont
{
namespace detail
{
struct EnabledDeviceTimerImpls;
}

/// A class that can be used to time operations in SVTK-m that might be occuring
/// in parallel. Users are recommended to provide a device adapter at construction
/// time which matches the one being used to execute algorithms to ensure that thread
/// synchronization is correct and accurate.
/// If no device adapter is provided at construction time, the maximum
/// elapsed time of all enabled deivces will be returned. Normally cuda is expected to
/// have the longest execution time if enabled.
/// Per device adapter time query is also supported. It's useful when users want to reuse
/// the same timer to measure the cuda kernal call as well as the cuda device execution.
/// It is also possible to change the device adapter after construction by calling the form
/// of the Reset method with a new DeviceAdapterId.
///
/// The there is no guaranteed resolution of the time but should generally be
/// good to about a millisecond.
///
class SVTKM_CONT_EXPORT Timer
{
public:
  SVTKM_CONT
  Timer();

  SVTKM_CONT Timer(svtkm::cont::DeviceAdapterId device);

  SVTKM_CONT ~Timer();

  /// Resets the timer.
  SVTKM_CONT void Reset();

  /// Resets the timer and changes the device to time on.
  SVTKM_CONT void Reset(svtkm::cont::DeviceAdapterId device);

  SVTKM_CONT void Start();

  SVTKM_CONT void Stop();

  SVTKM_CONT bool Started() const;

  SVTKM_CONT bool Stopped() const;

  /// Used to check if Timer has finished the synchronization to get the result from the device.
  SVTKM_CONT bool Ready() const;

  /// Get the elapsed time measured by the given device adapter. If no device is
  /// specified, the max time of all device measurements will be returned.
  SVTKM_CONT
  svtkm::Float64 GetElapsedTime() const;

  /// Returns the device for which this timer is synchronized. If the device adapter has the same
  /// id as DeviceAdapterTagAny, then the timer will synchronize all devices.
  SVTKM_CONT svtkm::cont::DeviceAdapterId GetDevice() const { return this->Device; }

private:
  /// Some timers are ill-defined when copied, so disallow that for all timers.
  SVTKM_CONT Timer(const Timer&) = delete;
  SVTKM_CONT void operator=(const Timer&) = delete;

  DeviceAdapterId Device;
  std::unique_ptr<detail::EnabledDeviceTimerImpls> Internal;
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_Timer_h
