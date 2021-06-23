//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Benchmarker.h"

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/internal/Configure.h>

#include <svtkm/testing/Testing.h>

#include <svtkm/List.h>

#include <sstream>

#ifdef SVTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif // TBB

// For the TBB implementation, the number of threads can be customized using a
// "NumThreads [numThreads]" argument.

namespace
{

// Make this global so benchmarks can access the current device id:
svtkm::cont::InitializeResult Config;

const svtkm::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const svtkm::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TypeList = svtkm::List<svtkm::UInt8,
                            svtkm::Vec2ui_8,
                            svtkm::Vec3ui_8,
                            svtkm::Vec4ui_8,
                            svtkm::UInt32,
                            svtkm::Vec2ui_32,
                            svtkm::UInt64,
                            svtkm::Vec2ui_64,
                            svtkm::Float32,
                            svtkm::Vec2f_32,
                            svtkm::Float64,
                            svtkm::Vec2f_64,
                            svtkm::Pair<svtkm::UInt32, svtkm::Float32>,
                            svtkm::Pair<svtkm::UInt32, svtkm::Float64>,
                            svtkm::Pair<svtkm::UInt64, svtkm::Float32>,
                            svtkm::Pair<svtkm::UInt64, svtkm::Float64>>;

template <typename ValueType>
void CopySpeed(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  state.SetLabel(svtkm::cont::GetHumanReadableSize(numBytes));

  svtkm::cont::ArrayHandle<ValueType> src;
  svtkm::cont::ArrayHandle<ValueType> dst;
  src.Allocate(numValues);
  dst.Allocate(numValues);

  svtkm::cont::Timer timer(device);
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    svtkm::cont::Algorithm::Copy(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(CopySpeed,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse SVTK-m options:
  auto opts = svtkm::cont::InitializeOptions::RequireDevice | svtkm::cont::InitializeOptions::AddHelp;
  Config = svtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

// Handle NumThreads command-line arg:
#ifdef SVTKM_ENABLE_TBB
  int numThreads = tbb::task_scheduler_init::automatic;
#endif // TBB

  if (argc == 3)
  {
    if (std::string(argv[1]) == "NumThreads")
    {
#ifdef SVTKM_ENABLE_TBB
      std::istringstream parse(argv[2]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " TBB threads." << std::endl;
#else
      std::cerr << "NumThreads valid only on TBB. Ignoring." << std::endl;
#endif // TBB
    }
  }

#ifdef SVTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init(numThreads);
#endif // TBB

  // handle benchmarking related args and run benchmarks:
  SVTKM_EXECUTE_BENCHMARKS(argc, argv);
}
