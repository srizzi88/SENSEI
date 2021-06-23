//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DeviceAdapterList.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

using TimerTestDevices =
  svtkm::ListAppend<SVTKM_DEFAULT_DEVICE_ADAPTER_LIST, svtkm::List<svtkm::cont::DeviceAdapterTagAny>>;

constexpr long long waitTimeMilliseconds = 250;
constexpr svtkm::Float64 waitTimeSeconds = svtkm::Float64(waitTimeMilliseconds) / 1000;

struct Waiter
{
  std::chrono::high_resolution_clock::time_point Start = std::chrono::high_resolution_clock::now();
  long long ExpectedTimeMilliseconds = 0;

  svtkm::Float64 Wait()
  {
    // Update when we want to wait to.
    this->ExpectedTimeMilliseconds += waitTimeMilliseconds;
    svtkm::Float64 expectedTimeSeconds = svtkm::Float64(this->ExpectedTimeMilliseconds) / 1000;

    long long elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - this->Start)
                                      .count();

    long long millisecondsToSleep = this->ExpectedTimeMilliseconds - elapsedMilliseconds;

    std::cout << "  Sleeping for " << millisecondsToSleep << "ms (to " << expectedTimeSeconds
              << "s)" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));

    SVTKM_TEST_ASSERT(std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now() - this->Start)
                         .count() <
                       (this->ExpectedTimeMilliseconds + ((3 * waitTimeMilliseconds) / 4)),
                     "Internal test error: Sleep lasted longer than expected. System must be busy. "
                     "Might need to increase waitTimeMilliseconds.");

    return expectedTimeSeconds;
  }
};

void CheckTime(const svtkm::cont::Timer& timer, svtkm::Float64 expectedTime)
{
  svtkm::Float64 elapsedTime = timer.GetElapsedTime();
  SVTKM_TEST_ASSERT(
    elapsedTime > (expectedTime - 0.001), "Timer did not capture full wait. ", elapsedTime);
  SVTKM_TEST_ASSERT(elapsedTime < (expectedTime + waitTimeSeconds),
                   "Timer counted too far or system really busy. ",
                   elapsedTime);
}

void DoTimerCheck(svtkm::cont::Timer& timer)
{
  std::cout << "  Starting timer" << std::endl;
  timer.Start();
  SVTKM_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
  SVTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track non stopped status");

  Waiter waiter;

  svtkm::Float64 expectedTime = 0.0;
  CheckTime(timer, expectedTime);

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Make sure timer is still running" << std::endl;
  SVTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track stopped status");

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Stop the timer" << std::endl;
  timer.Stop();
  SVTKM_TEST_ASSERT(timer.Stopped(), "Timer fails to track stopped status");

  CheckTime(timer, expectedTime);

  waiter.Wait(); // Do not advanced expected time

  std::cout << "  Check that timer legitimately stopped" << std::endl;
  CheckTime(timer, expectedTime);
}

struct TimerCheckFunctor
{
  void operator()(svtkm::cont::DeviceAdapterId device) const
  {
    if ((device != svtkm::cont::DeviceAdapterTagAny()) &&
        !svtkm::cont::GetRuntimeDeviceTracker().CanRunOn(device))
    {
      // A timer will not work if set on a device that is not supported. Just skip this test.
      return;
    }

    {
      svtkm::cont::Timer timer(device);
      DoTimerCheck(timer);
    }
    {
      svtkm::cont::Timer timer;
      timer.Reset(device);
      DoTimerCheck(timer);
    }
    {
      svtkm::cont::GetRuntimeDeviceTracker().DisableDevice(device);
      svtkm::cont::Timer timer(device);
      svtkm::cont::GetRuntimeDeviceTracker().ResetDevice(device);
      DoTimerCheck(timer);
    }
    {
      svtkm::cont::ScopedRuntimeDeviceTracker scoped(device);
      svtkm::cont::Timer timer(device);
      timer.Start();
      SVTKM_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
      //simulate a device failing
      scoped.DisableDevice(device);
      Waiter waiter;
      waiter.Wait();
      CheckTime(timer, 0.0);
    }
  }
};

void DoTimerTest()
{
  std::cout << "Check default timer" << std::endl;
  svtkm::cont::Timer timer;
  DoTimerCheck(timer);

  svtkm::ListForEach(TimerCheckFunctor(), TimerTestDevices());
}

} // anonymous namespace

int UnitTestTimer(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(DoTimerTest, argc, argv);
}
