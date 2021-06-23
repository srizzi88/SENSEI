//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_RuntimeDeviceTracker_h
#define svtk_m_cont_RuntimeDeviceTracker_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorBadDevice.h>
#include <svtkm/cont/RuntimeDeviceInformation.h>

#include <memory>

namespace svtkm
{
namespace cont
{
namespace detail
{

struct RuntimeDeviceTrackerInternals;
}
struct ScopedRuntimeDeviceTracker;

/// RuntimeDeviceTracker is the central location for determining
/// which device adapter will be active for algorithm execution.
/// Many features in SVTK-m will attempt to run algorithms on the "best
/// available device." This generally is determined at runtime as some
/// backends require specific hardware, or failures in one device are
/// recorded and that device is disabled.
///
/// While svtkm::cont::RunimeDeviceInformation reports on the existence
/// of a device being supported, this tracks on a per-thread basis
/// when worklets fail, why the fail, and will update the list
/// of valid runtime devices based on that information.
///
///
class SVTKM_CONT_EXPORT RuntimeDeviceTracker
{
  friend SVTKM_CONT_EXPORT svtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();

public:
  SVTKM_CONT
  ~RuntimeDeviceTracker();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  SVTKM_CONT bool CanRunOn(DeviceAdapterId deviceId) const;

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations.
  ///
  SVTKM_CONT void ReportAllocationFailure(svtkm::cont::DeviceAdapterId deviceId,
                                         const svtkm::cont::ErrorBadAllocation&)
  {
    this->SetDeviceState(deviceId, false);
  }


  /// Report a ErrorBadDevice failure and flag the device as unusable.
  SVTKM_CONT void ReportBadDeviceFailure(svtkm::cont::DeviceAdapterId deviceId,
                                        const svtkm::cont::ErrorBadDevice&)
  {
    this->SetDeviceState(deviceId, false);
  }

  /// Reset the tracker for the given device. This will discard any updates
  /// caused by reported failures. Passing DeviceAdapterTagAny to this will
  /// reset all devices ( same as \c Reset ).
  ///
  SVTKM_CONT void ResetDevice(svtkm::cont::DeviceAdapterId deviceId);

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  SVTKM_CONT
  void Reset();

  /// \brief Disable the given device
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for SVTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable (turn off) a given device.
  /// Use \c ResetDevice to turn the device back on (if it is supported).
  ///
  /// Passing DeviceAdapterTagAny to this will disable all devices
  ///
  SVTKM_CONT void DisableDevice(DeviceAdapterId deviceId);

  /// \brief Disable all devices except the specified one.
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for SVTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable all devices except one
  /// to effectively force SVTK-m to use that device. Either pass the
  /// DeviceAdapterTagAny to this function or call \c Reset to restore
  /// all devices to their default state.
  ///
  /// This method will throw a \c ErrorBadValue if the given device does not
  /// exist on the system.
  ///
  SVTKM_CONT void ForceDevice(DeviceAdapterId deviceId);

  SVTKM_CONT void PrintSummary(std::ostream& out) const;

private:
  friend struct ScopedRuntimeDeviceTracker;

  detail::RuntimeDeviceTrackerInternals* Internals;

  SVTKM_CONT
  RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details, bool reset);

  SVTKM_CONT
  RuntimeDeviceTracker(const RuntimeDeviceTracker&) = delete;

  SVTKM_CONT
  RuntimeDeviceTracker& operator=(const RuntimeDeviceTracker&) = delete;

  SVTKM_CONT
  void CheckDevice(svtkm::cont::DeviceAdapterId deviceId) const;

  SVTKM_CONT
  void SetDeviceState(svtkm::cont::DeviceAdapterId deviceId, bool state);

  SVTKM_CONT
  void LogEnabledDevices() const;
};


enum struct RuntimeDeviceTrackerMode
{
  Force,
  Enable,
  Disable
};

/// A class that can be used to determine or modify which device adapter
/// SVTK-m algorithms should be run on. This class captures the state
/// of the per-thread device adapter and will revert any changes applied
/// during its lifetime on destruction.
///
///
struct SVTKM_CONT_EXPORT ScopedRuntimeDeviceTracker : public svtkm::cont::RuntimeDeviceTracker
{
  /// Construct a ScopedRuntimeDeviceTracker where the state of the active devices
  /// for the current thread are determined by the parameters to the constructor.
  ///
  /// 'Force'
  ///   - Force-Enable the provided single device adapter
  ///   - Force-Enable all device adapters when using svtkm::cont::DeviceAdaterTagAny
  /// 'Enable'
  ///   - Enable the provided single device adapter if it was previously disabled
  ///   - Enable all device adapters that are currently disabled when using
  ///     svtkm::cont::DeviceAdaterTagAny
  /// 'Disable'
  ///   - Disable the provided single device adapter
  ///   - Disable all device adapters when using svtkm::cont::DeviceAdaterTagAny
  ///
  /// Constructor is not thread safe
  SVTKM_CONT ScopedRuntimeDeviceTracker(
    svtkm::cont::DeviceAdapterId device,
    RuntimeDeviceTrackerMode mode = RuntimeDeviceTrackerMode::Force);

  /// Construct a ScopedRuntimeDeviceTracker associated with the thread
  /// associated with the provided tracker. The active devices
  /// for the current thread are determined by the parameters to the constructor.
  ///
  /// 'Force'
  ///   - Force-Enable the provided single device adapter
  ///   - Force-Enable all device adapters when using svtkm::cont::DeviceAdaterTagAny
  /// 'Enable'
  ///   - Enable the provided single device adapter if it was previously disabled
  ///   - Enable all device adapters that are currently disabled when using
  ///     svtkm::cont::DeviceAdaterTagAny
  /// 'Disable'
  ///   - Disable the provided single device adapter
  ///   - Disable all device adapters when using svtkm::cont::DeviceAdaterTagAny
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread which ScopedRuntimeDeviceTracker was constructed on.
  ///
  /// Constructor is not thread safe
  SVTKM_CONT ScopedRuntimeDeviceTracker(svtkm::cont::DeviceAdapterId device,
                                       RuntimeDeviceTrackerMode mode,
                                       const svtkm::cont::RuntimeDeviceTracker& tracker);

  /// Construct a ScopedRuntimeDeviceTracker associated with the thread
  /// associated with the provided tracker.
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread which ScopedRuntimeDeviceTracker was constructed on.
  ///
  /// Constructor is not thread safe
  SVTKM_CONT ScopedRuntimeDeviceTracker(const svtkm::cont::RuntimeDeviceTracker& tracker);

  /// Destructor is not thread safe
  SVTKM_CONT ~ScopedRuntimeDeviceTracker();

private:
  std::unique_ptr<detail::RuntimeDeviceTrackerInternals> SavedState;
};

/// \brief Get the \c RuntimeDeviceTracker for the current thread.
///
/// Many features in SVTK-m will attempt to run algorithms on the "best
/// available device." This often is determined at runtime as failures in
/// one device are recorded and that device is disabled. To prevent having
/// to check over and over again, SVTK-m uses per thread runtime device tracker
/// so that these choices are marked and shared.
///
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();
}
} // namespace svtkm::cont

#endif //svtk_m_filter_RuntimeDeviceTracker_h
