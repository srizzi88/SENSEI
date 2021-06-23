//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_DeviceAdapterListHelpers_h
#define svtk_m_cont_internal_DeviceAdapterListHelpers_h

#include <svtkm/List.h>
#include <svtkm/cont/ErrorBadDevice.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

//============================================================================
struct ExecuteIfValidDeviceTag
{

  template <typename DeviceAdapter>
  using EnableIfValid = std::enable_if<DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter>
  using EnableIfInvalid = std::enable_if<!DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter, typename Functor, typename... Args>
  typename EnableIfValid<DeviceAdapter>::type operator()(
    DeviceAdapter device,
    Functor&& f,
    const svtkm::cont::RuntimeDeviceTracker& tracker,
    Args&&... args) const
  {
    if (tracker.CanRunOn(device))
    {
      f(device, std::forward<Args>(args)...);
    }
  }

  // do not generate code for invalid devices
  template <typename DeviceAdapter, typename... Args>
  typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter, Args&&...) const
  {
  }
};

/// Execute the given functor on each valid device in \c DeviceList.
///
template <typename DeviceList, typename Functor, typename... Args>
SVTKM_CONT void ForEachValidDevice(DeviceList devices, Functor&& functor, Args&&... args)
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  svtkm::ListForEach(
    ExecuteIfValidDeviceTag{}, devices, functor, tracker, std::forward<Args>(args)...);
}
}
}
} // svtkm::cont::internal

#endif // svtk_m_cont_internal_DeviceAdapterListHelpers_h
