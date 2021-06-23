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
#include <svtkm/cont/AtomicArray.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/TypeTraits.h>

#include <sstream>
#include <string>

namespace
{

// Provide access to the requested device to the benchmark functions:
svtkm::cont::InitializeResult Config;

// Range for array sizes
static constexpr svtkm::Id ARRAY_SIZE_MIN = 1;
static constexpr svtkm::Id ARRAY_SIZE_MAX = 1 << 20;

// This is 32x larger than the largest array size.
static constexpr svtkm::Id NUM_WRITES = 33554432; // 2^25

static constexpr svtkm::Id STRIDE = 32;

// Benchmarks AtomicArray::Add such that each work index writes to adjacent indices.
struct AddSeqWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& val, AtomicPortal& portal) const
  {
    portal.Add(i % portal.GetNumberOfValues(), val);
  }
};

template <typename ValueType>
void BenchAddSeq(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> atomicArray;
  svtkm::cont::Algorithm::Fill(
    atomicArray, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddSeqWorker{}, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchAddSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              svtkm::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchAddSeq
struct AddSeqBaselineWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& val, Portal& portal) const
  {
    const svtkm::Id j = i % portal.GetNumberOfValues();
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddSeqBaseline(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> array;
  svtkm::cont::Algorithm::Fill(array, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddSeqBaselineWorker{}, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchAddSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              svtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::Add such that each work index writes to a strided
// index ( floor(i / stride) + stride * (i % stride)
struct AddStrideWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  svtkm::Id Stride;

  AddStrideWorker(svtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& val, AtomicPortal& portal) const
  {
    const svtkm::Id numVals = portal.GetNumberOfValues();
    const svtkm::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Add(j, val);
  }
};

template <typename ValueType>
void BenchAddStride(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));
  const svtkm::Id stride = static_cast<svtkm::Id>(state.range(2));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> atomicArray;
  svtkm::cont::Algorithm::Fill(
    atomicArray, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddStrideWorker{ stride }, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  svtkm::cont::AtomicArrayTypeList);

// Non-atomic baseline for AddStride
struct AddStrideBaselineWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  svtkm::Id Stride;

  AddStrideBaselineWorker(svtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename Portal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& val, Portal& portal) const
  {
    const svtkm::Id numVals = portal.GetNumberOfValues();
    const svtkm::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddStrideBaseline(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));
  const svtkm::Id stride = static_cast<svtkm::Id>(state.range(2));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> array;
  svtkm::cont::Algorithm::Fill(array, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddStrideBaselineWorker{ stride }, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "Values", "Ops", "Stride" }),
  svtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareAndSwap such that each work index writes to adjacent
// indices.
struct CASSeqWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const svtkm::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    T assumed = static_cast<T>(0);
    do
    {
      assumed = oldVal;
      oldVal = portal.CompareAndSwap(idx, assumed + val, assumed);
    } while (assumed != oldVal);
  }
};

template <typename ValueType>
void BenchCASSeq(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> atomicArray;
  svtkm::cont::Algorithm::Fill(
    atomicArray, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASSeqWorker{}, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchCASSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              svtkm::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchCASSeq
struct CASSeqBaselineWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& in, Portal& portal) const
  {
    const svtkm::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    const T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASSeqBaseline(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> array;
  svtkm::cont::Algorithm::Fill(array, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASSeqBaselineWorker{}, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(BenchCASSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              svtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareAndSwap such that each work index writes to
// a strided index:
// ( floor(i / stride) + stride * (i % stride)
struct CASStrideWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  svtkm::Id Stride;

  CASStrideWorker(svtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const svtkm::Id numVals = portal.GetNumberOfValues();
    const svtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    T assumed = static_cast<T>(0);
    do
    {
      assumed = oldVal;
      oldVal = portal.CompareAndSwap(idx, assumed + val, assumed);
    } while (assumed != oldVal);
  }
};

template <typename ValueType>
void BenchCASStride(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));
  const svtkm::Id stride = static_cast<svtkm::Id>(state.range(2));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> atomicArray;
  svtkm::cont::Algorithm::Fill(
    atomicArray, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASStrideWorker{ stride }, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  svtkm::cont::AtomicArrayTypeList);

// Non-atomic baseline for CASStride
struct CASStrideBaselineWorker : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  svtkm::Id Stride;

  CASStrideBaselineWorker(svtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  SVTKM_EXEC void operator()(const svtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const svtkm::Id numVals = portal.GetNumberOfValues();
    const svtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASStrideBaseline(benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numValues = static_cast<svtkm::Id>(state.range(0));
  const svtkm::Id numWrites = static_cast<svtkm::Id>(state.range(1));
  const svtkm::Id stride = static_cast<svtkm::Id>(state.range(2));

  auto ones = svtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  svtkm::cont::ArrayHandle<ValueType> array;
  svtkm::cont::Algorithm::Fill(array, svtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  svtkm::cont::Invoker invoker{ device };
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASStrideBaselineWorker{ stride }, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
SVTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  svtkm::cont::AtomicArrayTypeList);

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
