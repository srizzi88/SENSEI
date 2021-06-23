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

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/worklet/WorkletMapField.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Make this global so benchmarks can access the current device id:
svtkm::cont::InitializeResult Config;

const svtkm::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const svtkm::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TestTypes = svtkm::List<svtkm::Float32>;

//------------- Functors for benchmarks --------------------------------------

// Reads all values in ArrayHandle.
struct ReadValues : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn);

  template <typename T>
  SVTKM_EXEC void operator()(const T& val) const
  {
    if (val < 0)
    {
      // We don't really do anything with this, we just need to do *something*
      // to prevent the compiler from optimizing out the array accesses.
      this->RaiseError("Unexpected value.");
    }
  }
};

// Writes values to ArrayHandle.
struct WriteValues : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  SVTKM_EXEC void operator()(T& val, svtkm::Id idx) const
  {
    val = static_cast<T>(idx);
  }
};

// Reads and writes values to ArrayHandle.
struct ReadWriteValues : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  SVTKM_EXEC void operator()(T& val, svtkm::Id idx) const
  {
    val += static_cast<T>(idx);
  }
};

//------------- Benchmark functors -------------------------------------------

// Copies NumValues from control environment to execution environment and
// accesses them as read-only.
template <typename ValueType>
void BenchContToExecRead(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-only): " << numValues << " values ("
         << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = svtkm::cont::make_ArrayHandle(vec);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(ReadValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Writes values to ArrayHandle in execution environment. There is no actual
// copy between control/execution in this case.
template <typename ValueType>
void BenchContToExecWrite(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution (write-only): " << numValues << " values ("
         << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(WriteValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// both reads and writes them.
template <typename ValueType>
void BenchContToExecReadWrite(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-write): " << numValues << " values ("
         << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = svtkm::cont::make_ArrayHandle(vec);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(ReadWriteValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then accesses them as read-only.
template <typename ValueType>
void BenchRoundTripRead(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-only): " << numValues
         << " values (" << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = svtkm::cont::make_ArrayHandle(vec);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Ensure data is in control before we start:
    array.ReleaseResourcesExecution();

    timer.Start();
    invoker(ReadValues{}, array);

    // Copy back to host and read:
    auto portal = array.GetPortalConstControl();
    for (svtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then reads and writes them in-place.
template <typename ValueType>
void BenchRoundTripReadWrite(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-write): " << numValues
         << " values (" << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = svtkm::cont::make_ArrayHandle(vec);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Ensure data is in control before we start:
    array.ReleaseResourcesExecution();

    timer.Start();

    // Do work on device:
    invoker(ReadWriteValues{}, array);

    auto portal = array.GetPortalControl();
    for (svtkm::Id i = 0; i < numValues; ++i)
    {
      portal.Set(i, portal.Get(i) - static_cast<ValueType>(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading.
template <typename ValueType>
void BenchExecToContRead(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-only on control): " << numValues
         << " values (" << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Time the copy:
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (svtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// and overwrites them.
template <typename ValueType>
void BenchExecToContWrite(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (write-only on control): " << numValues
         << " values (" << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (svtkm::Id i = 0; i < numValues; ++i)
    {
      portal.Set(i, portal.Get(i) - static_cast<ValueType>(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading and writing.
template <typename ValueType>
void BenchExecToContReadWrite(benchmark::State& state)
{
  using ArrayType = svtkm::cont::ArrayHandle<ValueType>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::UInt64 numBytes = static_cast<svtkm::UInt64>(state.range(0));
  const svtkm::Id numValues = static_cast<svtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-write on control): " << numValues
         << " values (" << svtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (svtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse SVTK-m options:
  auto opts = svtkm::cont::InitializeOptions::RequireDevice | svtkm::cont::InitializeOptions::AddHelp;
  Config = svtkm::cont::Initialize(argc, argv, opts);

  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  SVTKM_EXECUTE_BENCHMARKS(argc, argv);
}
