//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
#define svtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/ErrorExecution.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <svtkm/cont/internal/IteratorFromArrayPortal.h>
#include <svtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <svtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <svtkm/cont/tbb/internal/FunctorsTBB.h>
#include <svtkm/cont/tbb/internal/ParallelSortTBB.h>

#include <svtkm/exec/tbb/internal/TaskTiling.h>

namespace svtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTBB>
  : svtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTBB>,
      svtkm::cont::DeviceAdapterTagTBB>
{
public:
  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static void Copy(const svtkm::cont::ArrayHandle<T, CIn>& input,
                             svtkm::cont::ArrayHandle<U, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    const svtkm::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagTBB());

    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, inSize);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  SVTKM_CONT static void CopyIf(const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    ::svtkm::NotZeroInitialized unary_predicate;
    CopyIf(input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  SVTKM_CONT static void CopyIf(const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::Id inputSize = input.GetNumberOfValues();
    SVTKM_ASSERT(inputSize == stencil.GetNumberOfValues());
    svtkm::Id outputSize =
      tbb::CopyIfPortals(input.PrepareForInput(DeviceAdapterTagTBB()),
                         stencil.PrepareForInput(DeviceAdapterTagTBB()),
                         output.PrepareForOutput(inputSize, DeviceAdapterTagTBB()),
                         unary_predicate);
    output.Shrink(outputSize);
  }

  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static bool CopySubRange(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::Id inputStartIndex,
                                     svtkm::Id numberOfElementsToCopy,
                                     svtkm::cont::ArrayHandle<U, COut>& output,
                                     svtkm::Id outputIndex = 0)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    const svtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output && ((outputIndex >= inputStartIndex &&
                             outputIndex < inputStartIndex + numberOfElementsToCopy) ||
                            (inputStartIndex >= outputIndex &&
                             inputStartIndex < outputIndex + numberOfElementsToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfElementsToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfElementsToCopy))
    { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
    }

    const svtkm::Id outSize = output.GetNumberOfValues();
    const svtkm::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if (outSize < copyOutEnd)
    { //output is not large enough
      if (outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        svtkm::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagTBB());

    tbb::CopyPortals(
      inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfElementsToCopy);

    return true;
  }

  template <typename T, typename U, class CIn>
  SVTKM_CONT static U Reduce(const svtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return Reduce(input, initialValue, svtkm::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  SVTKM_CONT static U Reduce(const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return tbb::ReducePortals(
      input.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()), initialValue, binary_functor);
  }

  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  SVTKM_CONT static void ReduceByKey(const svtkm::cont::ArrayHandle<T, CKeyIn>& keys,
                                    const svtkm::cont::ArrayHandle<U, CValIn>& values,
                                    svtkm::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                    svtkm::cont::ArrayHandle<U, CValOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::Id inputSize = keys.GetNumberOfValues();
    SVTKM_ASSERT(inputSize == values.GetNumberOfValues());
    svtkm::Id outputSize =
      tbb::ReduceByKeyPortals(keys.PrepareForInput(DeviceAdapterTagTBB()),
                              values.PrepareForInput(DeviceAdapterTagTBB()),
                              keys_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB()),
                              values_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB()),
                              binary_functor);
    keys_output.Shrink(outputSize);
    values_output.Shrink(outputSize);
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return tbb::ScanInclusivePortals(
      input.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), svtkm::cont::DeviceAdapterTagTBB()),
      svtkm::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return tbb::ScanInclusivePortals(
      input.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), svtkm::cont::DeviceAdapterTagTBB()),
      binary_functor);
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return tbb::ScanExclusivePortals(
      input.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), svtkm::cont::DeviceAdapterTagTBB()),
      svtkm::Add(),
      svtkm::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor,
                                   const T& initialValue)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return tbb::ScanExclusivePortals(
      input.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), svtkm::cont::DeviceAdapterTagTBB()),
      binary_functor,
      initialValue);
  }

  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::tbb::internal::TaskTiling1D& functor,
                                            svtkm::Id size);
  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::tbb::internal::TaskTiling3D& functor,
                                            svtkm::Id3 size);

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id numInstances)
  {
    SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
                   "Schedule TBB 1D: '%s'",
                   svtkm::cont::TypeToString(functor).c_str());

    svtkm::exec::tbb::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id3 rangeMax)
  {
    SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
                   "Schedule TBB 3D: '%s'",
                   svtkm::cont::TypeToString(functor).c_str());

    svtkm::exec::tbb::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  //1. We need functions for each of the following


  template <typename T, class Container>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Container>& values)
  {
    //this is required to get sort to work with zip handles
    std::less<T> lessOp;
    svtkm::cont::tbb::sort::parallel_sort(values, lessOp);
  }

  template <typename T, class Container, class BinaryCompare>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Container>& values,
                             BinaryCompare binary_compare)
  {
    svtkm::cont::tbb::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    svtkm::cont::tbb::sort::parallel_sort_bykey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    svtkm::cont::tbb::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    svtkm::Id outputSize =
      tbb::UniquePortals(values.PrepareForInPlace(DeviceAdapterTagTBB()), binary_compare);
    values.Shrink(outputSize);
  }

  SVTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

/// TBB contains its own high resolution timer.
///
template <>
class DeviceAdapterTimerImplementation<svtkm::cont::DeviceAdapterTagTBB>
{
public:
  SVTKM_CONT DeviceAdapterTimerImplementation() { this->Reset(); }

  SVTKM_CONT void Reset()
  {
    this->StartReady = false;
    this->StopReady = false;
  }

  SVTKM_CONT void Start()
  {
    this->Reset();
    this->StartTime = this->GetCurrentTime();
    this->StartReady = true;
  }

  SVTKM_CONT void Stop()
  {
    this->StopTime = this->GetCurrentTime();
    this->StopReady = true;
  }

  SVTKM_CONT bool Started() const { return this->StartReady; }

  SVTKM_CONT bool Stopped() const { return this->StopReady; }

  SVTKM_CONT bool Ready() const { return true; }

  SVTKM_CONT svtkm::Float64 GetElapsedTime() const
  {
    assert(this->StartReady);
    if (!this->StartReady)
    {
      SVTKM_LOG_S(svtkm::cont::LogLevel::Error,
                 "Start() function should be called first then trying to call Stop() and"
                 " GetElapsedTime().");
      return 0;
    }

    ::tbb::tick_count startTime = this->StartTime;
    ::tbb::tick_count stopTime = this->StopReady ? this->StopTime : this->GetCurrentTime();

    ::tbb::tick_count::interval_t elapsedTime = stopTime - startTime;

    return static_cast<svtkm::Float64>(elapsedTime.seconds());
  }

  SVTKM_CONT::tbb::tick_count GetCurrentTime() const
  {
    svtkm::cont::DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    return ::tbb::tick_count::now();
  }

private:
  bool StartReady;
  bool StopReady;
  ::tbb::tick_count StartTime;
  ::tbb::tick_count StopTime;
};

template <>
class DeviceTaskTypes<svtkm::cont::DeviceAdapterTagTBB>
{
public:
  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::tbb::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                          InvocationType& invocation,
                                                          svtkm::Id,
                                                          svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::tbb::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::tbb::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                          InvocationType& invocation,
                                                          svtkm::Id3,
                                                          svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::tbb::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
