//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/Timer.h>

#include <tuple>

namespace
{
template <typename Device>
using DeviceInvalid = std::integral_constant<bool, !Device::IsEnabled>;
using EnabledDeviceList = svtkm::ListRemoveIf<svtkm::cont::DeviceAdapterListCommon, DeviceInvalid>;

template <typename Device>
using DeviceTimerPtr = std::unique_ptr<svtkm::cont::DeviceAdapterTimerImplementation<Device>>;

using EnabledTimerImpls = svtkm::ListTransform<EnabledDeviceList, DeviceTimerPtr>;
using EnabledTimerImplTuple = svtkm::ListApply<EnabledTimerImpls, std::tuple>;

// C++11 does not support get tuple element by type. C++14 does support that.
// Get the index of a type in tuple elements
template <class T, class Tuple>
struct Index;

template <class T, template <typename...> class Container, class... Types>
struct Index<T, Container<T, Types...>>
{
  static const std::size_t value = 0;
};

template <class T, class U, template <typename...> class Container, class... Types>
struct Index<T, Container<U, Types...>>
{
  static const std::size_t value = 1 + Index<T, Container<Types...>>::value;
};

template <typename Device>
SVTKM_CONT inline
  typename std::tuple_element<Index<Device, EnabledDeviceList>::value, EnabledTimerImplTuple>::type&
  GetUniqueTimerPtr(Device, EnabledTimerImplTuple& enabledTimers)
{
  return std::get<Index<Device, EnabledDeviceList>::value>(enabledTimers);
}

struct InitFunctor
{
  template <typename Device>
  SVTKM_CONT void operator()(Device, EnabledTimerImplTuple& timerImpls)
  {
    //We don't use the runtime device tracker to very initializtion support
    //so that the following use case is supported:
    //
    // GetRuntimeDeviceTracker().Disable( openMP );
    // svtkm::cont::Timer timer; //tracks all active devices
    // GetRuntimeDeviceTracker().Enable( openMP );
    // timer.Start() //want to test openmp
    //
    // timer.GetElapsedTime()
    //
    // When `GetElapsedTime` is called we need to make sure that the OpenMP
    // device timer is safe to call. At the same time we still need to make
    // sure that we have the required runtime and not just compile time support
    // this is why we use `DeviceAdapterRuntimeDetector`
    bool haveRequiredRuntimeSupport = svtkm::cont::DeviceAdapterRuntimeDetector<Device>{}.Exists();
    if (haveRequiredRuntimeSupport)
    {
      std::get<Index<Device, EnabledDeviceList>::value>(timerImpls)
        .reset(new svtkm::cont::DeviceAdapterTimerImplementation<Device>());
    }
  }
};

struct ResetFunctor
{
  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Reset();
    }
  }
};

struct StartFunctor
{
  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Start();
    }
  }
};

struct StopFunctor
{
  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      GetUniqueTimerPtr(device, timerImpls)->Stop();
    }
  }
};

struct StartedFunctor
{
  bool Value = true;

  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Started();
    }
  }
};

struct StoppedFunctor
{
  bool Value = true;

  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Stopped();
    }
  }
};

struct ReadyFunctor
{
  bool Value = true;

  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->Value &= GetUniqueTimerPtr(device, timerImpls)->Ready();
    }
  }
};

struct ElapsedTimeFunctor
{
  svtkm::Float64 ElapsedTime = 0.0;

  template <typename Device>
  SVTKM_CONT void operator()(Device device,
                            svtkm::cont::DeviceAdapterId deviceToRunOn,
                            const svtkm::cont::RuntimeDeviceTracker& tracker,
                            EnabledTimerImplTuple& timerImpls)
  {
    if ((deviceToRunOn == device || deviceToRunOn == svtkm::cont::DeviceAdapterTagAny()) &&
        tracker.CanRunOn(device))
    {
      this->ElapsedTime =
        svtkm::Max(this->ElapsedTime, GetUniqueTimerPtr(device, timerImpls)->GetElapsedTime());
    }
  }
};
} // anonymous namespace


namespace svtkm
{
namespace cont
{
namespace detail
{

struct EnabledDeviceTimerImpls
{
  EnabledDeviceTimerImpls()
  {
    svtkm::ListForEach(InitFunctor(), EnabledDeviceList(), this->EnabledTimers);
  }
  ~EnabledDeviceTimerImpls() {}
  // A tuple of enabled timer implementations
  EnabledTimerImplTuple EnabledTimers;
};
}
}
} // namespace svtkm::cont::detail


namespace svtkm
{
namespace cont
{

Timer::Timer()
  : Device(svtkm::cont::DeviceAdapterTagAny())
  , Internal(new detail::EnabledDeviceTimerImpls)
{
}

Timer::Timer(svtkm::cont::DeviceAdapterId device)
  : Device(device)
  , Internal(new detail::EnabledDeviceTimerImpls)
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  if (!tracker.CanRunOn(device))
  {
    SVTKM_LOG_S(svtkm::cont::LogLevel::Error,
               "Device '" << device.GetName() << "' can not run on current Device."
                                                 "Thus timer is not usable");
  }
}

Timer::~Timer() = default;

void Timer::Reset()
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  svtkm::ListForEach(
    ResetFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

void Timer::Reset(svtkm::cont::DeviceAdapterId device)
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  if (!tracker.CanRunOn(device))
  {
    SVTKM_LOG_S(svtkm::cont::LogLevel::Error,
               "Device '" << device.GetName() << "' can not run on current Device."
                                                 "Thus timer is not usable");
  }

  this->Device = device;
  this->Reset();
}

void Timer::Start()
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  svtkm::ListForEach(
    StartFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

void Timer::Stop()
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  svtkm::ListForEach(
    StopFunctor(), EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
}

bool Timer::Started() const
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  StartedFunctor functor;
  svtkm::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

bool Timer::Stopped() const
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  StoppedFunctor functor;
  svtkm::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

bool Timer::Ready() const
{
  const auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  ReadyFunctor functor;
  svtkm::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);
  return functor.Value;
}

svtkm::Float64 Timer::GetElapsedTime() const
{
  //Throw an exception if a timer bound device now can't be used
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();

  ElapsedTimeFunctor functor;
  svtkm::ListForEach(
    functor, EnabledDeviceList(), this->Device, tracker, this->Internal->EnabledTimers);

  return functor.ElapsedTime;
}
}
} // namespace svtkm::cont
