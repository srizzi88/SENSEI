//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Algorithm_h
#define svtk_m_cont_Algorithm_h

#include <svtkm/Types.h>

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/cont/internal/ArrayManagerExecution.h>


namespace svtkm
{
namespace cont
{

namespace detail
{
template <typename Device, typename T>
inline auto DoPrepareArgForExec(T&& object, std::true_type)
  -> decltype(std::declval<T>().PrepareForExecution(Device()))
{
  SVTKM_IS_EXECUTION_OBJECT(T);
  return object.PrepareForExecution(Device{});
}

template <typename Device, typename T>
inline T&& DoPrepareArgForExec(T&& object, std::false_type)
{
  static_assert(!svtkm::cont::internal::IsExecutionObjectBase<T>::value,
                "Internal error: failed to detect execution object.");
  return std::forward<T>(object);
}

template <typename Device, typename T>
auto PrepareArgForExec(T&& object)
  -> decltype(DoPrepareArgForExec<Device>(std::forward<T>(object),
                                          svtkm::cont::internal::IsExecutionObjectBase<T>{}))
{
  return DoPrepareArgForExec<Device>(std::forward<T>(object),
                                     svtkm::cont::internal::IsExecutionObjectBase<T>{});
}

struct BitFieldToUnorderedSetFunctor
{
  svtkm::Id Result{ 0 };

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    this->Result = svtkm::cont::DeviceAdapterAlgorithm<Device>::BitFieldToUnorderedSet(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CopyFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CopyIfFunctor
{

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CopySubRangeFunctor
{
  bool valid;

  CopySubRangeFunctor()
    : valid(false)
  {
  }

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    valid = svtkm::cont::DeviceAdapterAlgorithm<Device>::CopySubRange(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CountSetBitsFunctor
{
  svtkm::Id PopCount{ 0 };

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    this->PopCount = svtkm::cont::DeviceAdapterAlgorithm<Device>::CountSetBits(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct FillFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Fill(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct LowerBoundsFunctor
{

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename U>
struct ReduceFunctor
{
  U result;

  ReduceFunctor()
    : result(svtkm::TypeTraits<U>::ZeroInitialization())
  {
  }

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = svtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ReduceByKeyFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename U>
struct ScanInclusiveResultFunctor
{
  U result;

  ScanInclusiveResultFunctor()
    : result(svtkm::TypeTraits<U>::ZeroInitialization())
  {
  }

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename T>
struct StreamingScanExclusiveFunctor
{
  T result;
  StreamingScanExclusiveFunctor()
    : result(T(0))
  {
  }

  template <typename Device, class CIn, class COut>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::Id numBlocks,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result =
      svtkm::cont::DeviceAdapterAlgorithm<Device>::StreamingScanExclusive(numBlocks, input, output);
    return true;
  }

  template <typename Device, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::Id numBlocks,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            svtkm::cont::ArrayHandle<T, COut>& output,
                            BinaryFunctor binary_functor,
                            const T& initialValue)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = svtkm::cont::DeviceAdapterAlgorithm<Device>::StreamingScanExclusive(
      numBlocks, input, output, binary_functor, initialValue);
    return true;
  }
};

template <typename U>
struct StreamingReduceFunctor
{
  U result;
  StreamingReduceFunctor()
    : result(U(0))
  {
  }
  template <typename Device, typename T, class CIn>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::Id numBlocks,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result =
      svtkm::cont::DeviceAdapterAlgorithm<Device>::StreamingReduce(numBlocks, input, initialValue);
    return true;
  }

  template <typename Device, typename T, class CIn, class BinaryFunctor>
  SVTKM_CONT bool operator()(Device,
                            const svtkm::Id numBlocks,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U InitialValue,
                            BinaryFunctor binaryFunctor)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = svtkm::cont::DeviceAdapterAlgorithm<Device>::StreamingReduce(
      numBlocks, input, InitialValue, binaryFunctor);
    return true;
  }
};

struct ScanInclusiveByKeyFunctor
{
  ScanInclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename T>
struct ScanExclusiveFunctor
{
  T result;

  ScanExclusiveFunctor()
    : result(T())
  {
  }

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ScanExclusiveByKeyFunctor
{
  ScanExclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename T>
struct ScanExtendedFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExtended(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ScheduleFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct SortFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct SortByKeyFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct SynchronizeFunctor
{
  template <typename Device>
  SVTKM_CONT bool operator()(Device)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Synchronize();
    return true;
  }
};

struct TransformFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Transform(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct UniqueFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Unique(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct UpperBoundsFunctor
{
  template <typename Device, typename... Args>
  SVTKM_CONT bool operator()(Device, Args&&... args) const
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    svtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};
} // namespace detail

struct Algorithm
{

  template <typename IndicesStorage>
  SVTKM_CONT static svtkm::Id BitFieldToUnorderedSet(
    svtkm::cont::DeviceAdapterId devId,
    const svtkm::cont::BitField& bits,
    svtkm::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    detail::BitFieldToUnorderedSetFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits, indices);
    return functor.Result;
  }

  template <typename IndicesStorage>
  SVTKM_CONT static svtkm::Id BitFieldToUnorderedSet(
    const svtkm::cont::BitField& bits,
    svtkm::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    detail::BitFieldToUnorderedSetFunctor functor;
    svtkm::cont::TryExecute(functor, bits, indices);
    return functor.Result;
  }

  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static bool Copy(svtkm::cont::DeviceAdapterId devId,
                             const svtkm::cont::ArrayHandle<T, CIn>& input,
                             svtkm::cont::ArrayHandle<U, COut>& output)
  {
    // If we can use any device, prefer to use source's already loaded device.
    if (devId == svtkm::cont::DeviceAdapterTagAny())
    {
      bool isCopied = svtkm::cont::TryExecuteOnDevice(
        input.GetDeviceAdapterId(), detail::CopyFunctor(), input, output);
      if (isCopied)
      {
        return true;
      }
    }
    return svtkm::cont::TryExecuteOnDevice(devId, detail::CopyFunctor(), input, output);
  }
  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static void Copy(const svtkm::cont::ArrayHandle<T, CIn>& input,
                             svtkm::cont::ArrayHandle<U, COut>& output)
  {
    Copy(svtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, typename U, class CIn, class CStencil, class COut>
  SVTKM_CONT static void CopyIf(svtkm::cont::DeviceAdapterId devId,
                               const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::CopyIfFunctor(), input, stencil, output);
  }
  template <typename T, typename U, class CIn, class CStencil, class COut>
  SVTKM_CONT static void CopyIf(const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output)
  {
    CopyIf(svtkm::cont::DeviceAdapterTagAny(), input, stencil, output);
  }


  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  SVTKM_CONT static void CopyIf(svtkm::cont::DeviceAdapterId devId,
                               const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::CopyIfFunctor(), input, stencil, output, unary_predicate);
  }
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  SVTKM_CONT static void CopyIf(const svtkm::cont::ArrayHandle<T, CIn>& input,
                               const svtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               svtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    CopyIf(svtkm::cont::DeviceAdapterTagAny(), input, stencil, output, unary_predicate);
  }


  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static bool CopySubRange(svtkm::cont::DeviceAdapterId devId,
                                     const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::Id inputStartIndex,
                                     svtkm::Id numberOfElementsToCopy,
                                     svtkm::cont::ArrayHandle<U, COut>& output,
                                     svtkm::Id outputIndex = 0)
  {
    detail::CopySubRangeFunctor functor;
    svtkm::cont::TryExecuteOnDevice(
      devId, functor, input, inputStartIndex, numberOfElementsToCopy, output, outputIndex);
    return functor.valid;
  }
  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static bool CopySubRange(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::Id inputStartIndex,
                                     svtkm::Id numberOfElementsToCopy,
                                     svtkm::cont::ArrayHandle<U, COut>& output,
                                     svtkm::Id outputIndex = 0)
  {
    return CopySubRange(svtkm::cont::DeviceAdapterTagAny(),
                        input,
                        inputStartIndex,
                        numberOfElementsToCopy,
                        output,
                        outputIndex);
  }

  SVTKM_CONT static svtkm::Id CountSetBits(svtkm::cont::DeviceAdapterId devId,
                                         const svtkm::cont::BitField& bits)
  {
    detail::CountSetBitsFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits);
    return functor.PopCount;
  }

  SVTKM_CONT static svtkm::Id CountSetBits(const svtkm::cont::BitField& bits)
  {
    return CountSetBits(svtkm::cont::DeviceAdapterTagAny{}, bits);
  }

  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::BitField& bits,
                             bool value,
                             svtkm::Id numBits)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits, value, numBits);
  }

  SVTKM_CONT static void Fill(svtkm::cont::BitField& bits, bool value, svtkm::Id numBits)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, bits, value, numBits);
  }

  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::BitField& bits,
                             bool value)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits, value);
  }

  SVTKM_CONT static void Fill(svtkm::cont::BitField& bits, bool value)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, bits, value);
  }

  template <typename WordType>
  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::BitField& bits,
                             WordType word,
                             svtkm::Id numBits)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits, word, numBits);
  }

  template <typename WordType>
  SVTKM_CONT static void Fill(svtkm::cont::BitField& bits, WordType word, svtkm::Id numBits)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, bits, word, numBits);
  }

  template <typename WordType>
  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::BitField& bits,
                             WordType word)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, bits, word);
  }

  template <typename WordType>
  SVTKM_CONT static void Fill(svtkm::cont::BitField& bits, WordType word)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, bits, word);
  }

  template <typename T, typename S>
  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::ArrayHandle<T, S>& handle,
                             const T& value)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, handle, value);
  }

  template <typename T, typename S>
  SVTKM_CONT static void Fill(svtkm::cont::ArrayHandle<T, S>& handle, const T& value)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, handle, value);
  }

  template <typename T, typename S>
  SVTKM_CONT static void Fill(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::ArrayHandle<T, S>& handle,
                             const T& value,
                             const svtkm::Id numValues)
  {
    detail::FillFunctor functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, handle, value, numValues);
  }

  template <typename T, typename S>
  SVTKM_CONT static void Fill(svtkm::cont::ArrayHandle<T, S>& handle,
                             const T& value,
                             const svtkm::Id numValues)
  {
    Fill(svtkm::cont::DeviceAdapterTagAny{}, handle, value, numValues);
  }

  template <typename T, class CIn, class CVal, class COut>
  SVTKM_CONT static void LowerBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  SVTKM_CONT static void LowerBounds(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output)
  {
    LowerBounds(svtkm::cont::DeviceAdapterTagAny(), input, values, output);
  }


  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  SVTKM_CONT static void LowerBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::LowerBoundsFunctor(), input, values, output, binary_compare);
  }
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  SVTKM_CONT static void LowerBounds(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    LowerBounds(svtkm::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  SVTKM_CONT static void LowerBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<svtkm::Id, CIn>& input,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& values_output)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values_output);
  }
  template <class CIn, class COut>
  SVTKM_CONT static void LowerBounds(const svtkm::cont::ArrayHandle<svtkm::Id, CIn>& input,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& values_output)
  {
    LowerBounds(svtkm::cont::DeviceAdapterTagAny(), input, values_output);
  }


  template <typename T, typename U, class CIn>
  SVTKM_CONT static U Reduce(svtkm::cont::DeviceAdapterId devId,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue)
  {
    detail::ReduceFunctor<U> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, initialValue);
    return functor.result;
  }
  template <typename T, typename U, class CIn>
  SVTKM_CONT static U Reduce(const svtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(svtkm::cont::DeviceAdapterTagAny(), input, initialValue);
  }


  template <typename T, typename U, class CIn, class BinaryFunctor>
  SVTKM_CONT static U Reduce(svtkm::cont::DeviceAdapterId devId,
                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    detail::ReduceFunctor<U> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, initialValue, binary_functor);
    return functor.result;
  }
  template <typename T, typename U, class CIn, class BinaryFunctor>
  SVTKM_CONT static U Reduce(const svtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    return Reduce(svtkm::cont::DeviceAdapterTagAny(), input, initialValue, binary_functor);
  }


  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  SVTKM_CONT static void ReduceByKey(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<T, CKeyIn>& keys,
                                    const svtkm::cont::ArrayHandle<U, CValIn>& values,
                                    svtkm::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                    svtkm::cont::ArrayHandle<U, CValOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    svtkm::cont::TryExecuteOnDevice(devId,
                                   detail::ReduceByKeyFunctor(),
                                   keys,
                                   values,
                                   keys_output,
                                   values_output,
                                   binary_functor);
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
    ReduceByKey(
      svtkm::cont::DeviceAdapterTagAny(), keys, values, keys_output, values_output, binary_functor);
  }


  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanInclusive(svtkm::cont::DeviceAdapterId devId,
                                   const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    return ScanInclusive(svtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, class CIn, class COut>
  SVTKM_CONT static T StreamingScanExclusive(svtkm::cont::DeviceAdapterId devId,
                                            const svtkm::Id numBlocks,
                                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                                            svtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::StreamingScanExclusiveFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, numBlocks, input, output);
    return functor.result;
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T StreamingScanExclusive(const svtkm::Id numBlocks,
                                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                                            svtkm::cont::ArrayHandle<T, COut>& output)
  {
    return StreamingScanExclusive(svtkm::cont::DeviceAdapterTagAny(), numBlocks, input, output);
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T StreamingScanExclusive(const svtkm::Id numBlocks,
                                            const svtkm::cont::ArrayHandle<T, CIn>& input,
                                            svtkm::cont::ArrayHandle<T, COut>& output,
                                            BinaryFunctor binary_functor,
                                            const T& initialValue)
  {
    detail::StreamingScanExclusiveFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(svtkm::cont::DeviceAdapterTagAny(),
                                   functor,
                                   numBlocks,
                                   input,
                                   output,
                                   binary_functor,
                                   initialValue);
    return functor.result;
  }

  template <typename T, typename U, class CIn>
  SVTKM_CONT static U StreamingReduce(const svtkm::Id numBlocks,
                                     const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     U initialValue)
  {
    detail::StreamingReduceFunctor<U> functor;
    svtkm::cont::TryExecuteOnDevice(
      svtkm::cont::DeviceAdapterTagAny(), functor, numBlocks, input, initialValue);
    return functor.result;
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  SVTKM_CONT static U StreamingReduce(const svtkm::Id numBlocks,
                                     const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     U initialValue,
                                     BinaryFunctor binaryFunctor)
  {
    detail::StreamingReduceFunctor<U> functor;
    svtkm::cont::TryExecuteOnDevice(
      svtkm::cont::DeviceAdapterTagAny(), functor, numBlocks, input, initialValue, binaryFunctor);
    return functor.result;
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanInclusive(svtkm::cont::DeviceAdapterId devId,
                                   const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output, binary_functor);
    return functor.result;
  }
  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    return ScanInclusive(svtkm::cont::DeviceAdapterTagAny(), input, output, binary_functor);
  }


  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  SVTKM_CONT static void ScanInclusiveByKey(svtkm::cont::DeviceAdapterId devId,
                                           const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& values_output,
                                           BinaryFunctor binary_functor)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output, binary_functor);
  }
  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  SVTKM_CONT static void ScanInclusiveByKey(const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& values_output,
                                           BinaryFunctor binary_functor)
  {
    ScanInclusiveByKey(
      svtkm::cont::DeviceAdapterTagAny(), keys, values, values_output, binary_functor);
  }


  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  SVTKM_CONT static void ScanInclusiveByKey(svtkm::cont::DeviceAdapterId devId,
                                           const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }
  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  SVTKM_CONT static void ScanInclusiveByKey(const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    ScanInclusiveByKey(svtkm::cont::DeviceAdapterTagAny(), keys, values, values_output);
  }


  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanExclusive(svtkm::cont::DeviceAdapterId devId,
                                   const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExclusiveFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    return ScanExclusive(svtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanExclusive(svtkm::cont::DeviceAdapterId devId,
                                   const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    detail::ScanExclusiveFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output, binaryFunctor, initialValue);
    return functor.result;
  }
  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    return ScanExclusive(
      svtkm::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }


  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  SVTKM_CONT static void ScanExclusiveByKey(svtkm::cont::DeviceAdapterId devId,
                                           const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binaryFunctor)
  {
    svtkm::cont::TryExecuteOnDevice(devId,
                                   detail::ScanExclusiveByKeyFunctor(),
                                   keys,
                                   values,
                                   output,
                                   initialValue,
                                   binaryFunctor);
  }
  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  SVTKM_CONT static void ScanExclusiveByKey(const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binaryFunctor)
  {
    ScanExclusiveByKey(
      svtkm::cont::DeviceAdapterTagAny(), keys, values, output, initialValue, binaryFunctor);
  }


  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  SVTKM_CONT static void ScanExclusiveByKey(svtkm::cont::DeviceAdapterId devId,
                                           const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& output)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanExclusiveByKeyFunctor(), keys, values, output);
  }
  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  SVTKM_CONT static void ScanExclusiveByKey(const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const svtkm::cont::ArrayHandle<U, VIn>& values,
                                           svtkm::cont::ArrayHandle<U, VOut>& output)
  {
    ScanExclusiveByKey(svtkm::cont::DeviceAdapterTagAny(), keys, values, output);
  }


  template <typename T, class CIn, class COut>
  SVTKM_CONT static void ScanExtended(svtkm::cont::DeviceAdapterId devId,
                                     const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExtendedFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output);
  }
  template <typename T, class CIn, class COut>
  SVTKM_CONT static void ScanExtended(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::cont::ArrayHandle<T, COut>& output)
  {
    ScanExtended(svtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static void ScanExtended(svtkm::cont::DeviceAdapterId devId,
                                     const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::cont::ArrayHandle<T, COut>& output,
                                     BinaryFunctor binaryFunctor,
                                     const T& initialValue)
  {
    detail::ScanExtendedFunctor<T> functor;
    svtkm::cont::TryExecuteOnDevice(devId, functor, input, output, binaryFunctor, initialValue);
  }
  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static void ScanExtended(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                     svtkm::cont::ArrayHandle<T, COut>& output,
                                     BinaryFunctor binaryFunctor,
                                     const T& initialValue)
  {
    ScanExtended(svtkm::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }


  template <class Functor>
  SVTKM_CONT static void Schedule(svtkm::cont::DeviceAdapterId devId,
                                 Functor functor,
                                 svtkm::Id numInstances)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, numInstances);
  }
  template <class Functor>
  SVTKM_CONT static void Schedule(Functor functor, svtkm::Id numInstances)
  {
    Schedule(svtkm::cont::DeviceAdapterTagAny(), functor, numInstances);
  }


  template <class Functor>
  SVTKM_CONT static void Schedule(svtkm::cont::DeviceAdapterId devId,
                                 Functor functor,
                                 svtkm::Id3 rangeMax)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, rangeMax);
  }
  template <class Functor>
  SVTKM_CONT static void Schedule(Functor functor, svtkm::Id3 rangeMax)
  {
    Schedule(svtkm::cont::DeviceAdapterTagAny(), functor, rangeMax);
  }


  template <typename T, class Storage>
  SVTKM_CONT static void Sort(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values);
  }
  template <typename T, class Storage>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Sort(svtkm::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Sort(svtkm::cont::DeviceAdapterId devId,
                             svtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    Sort(svtkm::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, typename U, class StorageT, class StorageU>
  SVTKM_CONT static void SortByKey(svtkm::cont::DeviceAdapterId devId,
                                  svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values);
  }
  template <typename T, typename U, class StorageT, class StorageU>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKey(svtkm::cont::DeviceAdapterTagAny(), keys, values);
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  SVTKM_CONT static void SortByKey(svtkm::cont::DeviceAdapterId devId,
                                  svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values, binary_compare);
  }
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  SVTKM_CONT static void SortByKey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  svtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    SortByKey(svtkm::cont::DeviceAdapterTagAny(), keys, values, binary_compare);
  }


  SVTKM_CONT static void Synchronize(svtkm::cont::DeviceAdapterId devId)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::SynchronizeFunctor());
  }
  SVTKM_CONT static void Synchronize() { Synchronize(svtkm::cont::DeviceAdapterTagAny()); }


  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  SVTKM_CONT static void Transform(svtkm::cont::DeviceAdapterId devId,
                                  const svtkm::cont::ArrayHandle<T, StorageT>& input1,
                                  const svtkm::cont::ArrayHandle<U, StorageU>& input2,
                                  svtkm::cont::ArrayHandle<V, StorageV>& output,
                                  BinaryFunctor binaryFunctor)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::TransformFunctor(), input1, input2, output, binaryFunctor);
  }
  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  SVTKM_CONT static void Transform(const svtkm::cont::ArrayHandle<T, StorageT>& input1,
                                  const svtkm::cont::ArrayHandle<U, StorageU>& input2,
                                  svtkm::cont::ArrayHandle<V, StorageV>& output,
                                  BinaryFunctor binaryFunctor)
  {
    Transform(svtkm::cont::DeviceAdapterTagAny(), input1, input2, output, binaryFunctor);
  }


  template <typename T, class Storage>
  SVTKM_CONT static void Unique(svtkm::cont::DeviceAdapterId devId,
                               svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values);
  }
  template <typename T, class Storage>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(svtkm::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Unique(svtkm::cont::DeviceAdapterId devId,
                               svtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Unique(svtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    Unique(svtkm::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, class CIn, class CVal, class COut>
  SVTKM_CONT static void UpperBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  SVTKM_CONT static void UpperBounds(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output)
  {
    UpperBounds(svtkm::cont::DeviceAdapterTagAny(), input, values, output);
  }


  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  SVTKM_CONT static void UpperBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    svtkm::cont::TryExecuteOnDevice(
      devId, detail::UpperBoundsFunctor(), input, values, output, binary_compare);
  }
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  SVTKM_CONT static void UpperBounds(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                    const svtkm::cont::ArrayHandle<T, CVal>& values,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    UpperBounds(svtkm::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  SVTKM_CONT static void UpperBounds(svtkm::cont::DeviceAdapterId devId,
                                    const svtkm::cont::ArrayHandle<svtkm::Id, CIn>& input,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& values_output)
  {
    svtkm::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values_output);
  }
  template <class CIn, class COut>
  SVTKM_CONT static void UpperBounds(const svtkm::cont::ArrayHandle<svtkm::Id, CIn>& input,
                                    svtkm::cont::ArrayHandle<svtkm::Id, COut>& values_output)
  {
    UpperBounds(svtkm::cont::DeviceAdapterTagAny(), input, values_output);
  }
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_Algorithm_h
