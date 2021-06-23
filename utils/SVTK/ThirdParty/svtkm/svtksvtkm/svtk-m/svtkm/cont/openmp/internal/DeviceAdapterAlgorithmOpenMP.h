//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
#define svtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h

#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Error.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <svtkm/cont/openmp/internal/FunctorsOpenMP.h>
#include <svtkm/cont/openmp/internal/ParallelScanOpenMP.h>
#include <svtkm/cont/openmp/internal/ParallelSortOpenMP.h>
#include <svtkm/exec/openmp/internal/TaskTilingOpenMP.h>

#include <omp.h>

#include <algorithm>
#include <type_traits>

namespace svtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagOpenMP>
  : svtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagOpenMP>,
      svtkm::cont::DeviceAdapterTagOpenMP>
{
  using DevTag = DeviceAdapterTagOpenMP;

public:
  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static void Copy(const svtkm::cont::ArrayHandle<T, CIn>& input,
                             svtkm::cont::ArrayHandle<U, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    using namespace svtkm::cont::openmp;

    const svtkm::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    auto inputPortal = input.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForOutput(inSize, DevTag());
    CopyHelper(inputPortal, outputPortal, 0, 0, inSize);
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

    using namespace svtkm::cont::openmp;

    svtkm::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    auto inputPortal = input.PrepareForInput(DevTag());
    auto stencilPortal = stencil.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForOutput(inSize, DevTag());

    auto inIter = svtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
    auto stencilIter = svtkm::cont::ArrayPortalToIteratorBegin(stencilPortal);
    auto outIter = svtkm::cont::ArrayPortalToIteratorBegin(outputPortal);

    CopyIfHelper helper;

    SVTKM_OPENMP_DIRECTIVE(parallel default(shared))
    {

      SVTKM_OPENMP_DIRECTIVE(single)
      {
        // Calls omp_get_num_threads, thus must be used inside a parallel section.
        helper.Initialize(inSize, sizeof(T));
      }

      SVTKM_OPENMP_DIRECTIVE(for schedule(static))
      for (svtkm::Id i = 0; i < helper.NumChunks; ++i)
      {
        helper.CopyIf(inIter, stencilIter, outIter, unary_predicate, i);
      }
    }

    svtkm::Id numValues = helper.Reduce(outIter);
    output.Shrink(numValues);
  }


  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static bool CopySubRange(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::Id inputStartIndex,
                                     svtkm::Id numberOfValuesToCopy,
                                     svtkm::cont::ArrayHandle<U, COut>& output,
                                     svtkm::Id outputIndex = 0)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    using namespace svtkm::cont::openmp;

    const svtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex && outputIndex < inputStartIndex + numberOfValuesToCopy) ||
         (inputStartIndex >= outputIndex && inputStartIndex < outputIndex + numberOfValuesToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfValuesToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfValuesToCopy))
    { //adjust the size
      numberOfValuesToCopy = (inSize - inputStartIndex);
    }

    const svtkm::Id outSize = output.GetNumberOfValues();
    const svtkm::Id copyOutEnd = outputIndex + numberOfValuesToCopy;
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

    auto inputPortal = input.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForInPlace(DevTag());

    CopyHelper(inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfValuesToCopy);

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

    using namespace svtkm::cont::openmp;

    auto portal = input.PrepareForInput(DevTag());
    const OpenMPReductionSupported<typename std::decay<U>::type> fastPath;

    return ReduceHelper::Execute(portal, initialValue, binary_functor, fastPath);
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
                                    BinaryFunctor func)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    openmp::ReduceByKeyHelper(keys, values, keys_output, values_output, func);
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return ScanInclusive(input, output, svtkm::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return svtkm::TypeTraits<T>::ZeroInitialization();
    }

    using InPortalT = decltype(input.PrepareForInput(DevTag()));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag()));
    using Impl = openmp::ScanInclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    svtkm::Id numVals = input.GetNumberOfValues();
    Impl impl(
      input.PrepareForInput(DevTag()), output.PrepareForOutput(numVals, DevTag()), binaryFunctor);

    return impl.Execute(svtkm::Id2(0, numVals));
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return ScanExclusive(input, output, svtkm::Add(), svtkm::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return initialValue;
    }

    using InPortalT = decltype(input.PrepareForInput(DevTag()));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag()));
    using Impl = openmp::ScanExclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    svtkm::Id numVals = input.GetNumberOfValues();
    Impl impl(input.PrepareForInput(DevTag()),
              output.PrepareForOutput(numVals, DevTag()),
              binaryFunctor,
              initialValue);

    return impl.Execute(svtkm::Id2(0, numVals));
  }

  /// \brief Unstable ascending sort of input array.
  ///
  /// Sorts the contents of \c values so that they in ascending value. Doesn't
  /// guarantee stability
  ///
  template <typename T, class Storage>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    Sort(values, svtkm::SortLess());
  }

  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    openmp::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    openmp::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    auto portal = values.PrepareForInPlace(DevTag());
    auto iter = svtkm::cont::ArrayPortalToIteratorBegin(portal);

    using IterT = typename std::decay<decltype(iter)>::type;
    using Uniqifier = openmp::UniqueHelper<IterT, BinaryCompare>;

    Uniqifier uniquifier(iter, portal.GetNumberOfValues(), binary_compare);
    svtkm::Id outSize = uniquifier.Execute();
    values.Shrink(outSize);
  }

  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::openmp::internal::TaskTiling1D& functor,
                                            svtkm::Id size);
  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::openmp::internal::TaskTiling3D& functor,
                                            svtkm::Id3 size);

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id numInstances)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::exec::openmp::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id3 rangeMax)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::exec::openmp::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  SVTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control thread is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

template <>
class DeviceTaskTypes<svtkm::cont::DeviceAdapterTagOpenMP>
{
public:
  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::openmp::internal::TaskTiling1D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             svtkm::Id,
                                                             svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::openmp::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::openmp::internal::TaskTiling3D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             svtkm::Id3,
                                                             svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::openmp::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
