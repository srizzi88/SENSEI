//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
#define svtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/ErrorExecution.h>
#include <svtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <svtkm/BinaryOperators.h>

#include <svtkm/exec/serial/internal/TaskTiling.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace svtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>
  : svtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>,
      svtkm::cont::DeviceAdapterTagSerial>
{
private:
  using Device = svtkm::cont::DeviceAdapterTagSerial;

  // MSVC likes complain about narrowing type conversions in std::copy and
  // provides no reasonable way to disable the warning. As a work-around, this
  // template calls std::copy if and only if the types match, otherwise falls
  // back to a iterative casting approach. Since std::copy can only really
  // optimize same-type copies, this shouldn't affect performance.
  template <typename InIter, typename OutIter>
  static void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::false_type)
  {
    using OutputType = typename std::iterator_traits<OutIter>::value_type;
    while (src != srcEnd)
    {
      *dst = static_cast<OutputType>(*src);
      ++src;
      ++dst;
    }
  }

  template <typename InIter, typename OutIter>
  static void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::true_type)
  {
    std::copy(src, srcEnd, dst);
  }

public:
  template <typename T, typename U, class CIn, class COut>
  SVTKM_CONT static void Copy(const svtkm::cont::ArrayHandle<T, CIn>& input,
                             svtkm::cont::ArrayHandle<U, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    const svtkm::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagSerial());

    if (inSize <= 0)
    {
      return;
    }

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(svtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
           svtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
           svtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
           std::is_same<InputType, OutputType>());
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
                               UnaryPredicate predicate)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::Id inputSize = input.GetNumberOfValues();
    SVTKM_ASSERT(inputSize == stencil.GetNumberOfValues());

    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto stencilPortal = stencil.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForOutput(inputSize, DeviceAdapterTagSerial());

    svtkm::Id readPos = 0;
    svtkm::Id writePos = 0;

    for (; readPos < inputSize; ++readPos)
    {
      if (predicate(stencilPortal.Get(readPos)))
      {
        outputPortal.Set(writePos, inputPortal.Get(readPos));
        ++writePos;
      }
    }

    output.Shrink(writePos);
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

    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial());
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagSerial());
    auto inIter = svtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
    auto outIter = svtkm::cont::ArrayPortalToIteratorBegin(outputPortal);

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(inIter + inputStartIndex,
           inIter + inputStartIndex + numberOfElementsToCopy,
           outIter + outputIndex,
           std::is_same<InputType, OutputType>());

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

    internal::WrappedBinaryOperator<U, BinaryFunctor> wrappedOp(binary_functor);
    auto inputPortal = input.PrepareForInput(Device());
    return std::accumulate(svtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                           svtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                           initialValue,
                           wrappedOp);
  }

  template <typename T,
            typename U,
            class KIn,
            class VIn,
            class KOut,
            class VOut,
            class BinaryFunctor>
  SVTKM_CONT static void ReduceByKey(const svtkm::cont::ArrayHandle<T, KIn>& keys,
                                    const svtkm::cont::ArrayHandle<U, VIn>& values,
                                    svtkm::cont::ArrayHandle<T, KOut>& keys_output,
                                    svtkm::cont::ArrayHandle<U, VOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    auto keysPortalIn = keys.PrepareForInput(Device());
    auto valuesPortalIn = values.PrepareForInput(Device());
    const svtkm::Id numberOfKeys = keys.GetNumberOfValues();

    SVTKM_ASSERT(numberOfKeys == values.GetNumberOfValues());
    if (numberOfKeys == 0)
    {
      keys_output.Shrink(0);
      values_output.Shrink(0);
      return;
    }

    auto keysPortalOut = keys_output.PrepareForOutput(numberOfKeys, Device());
    auto valuesPortalOut = values_output.PrepareForOutput(numberOfKeys, Device());

    svtkm::Id writePos = 0;
    svtkm::Id readPos = 0;

    T currentKey = keysPortalIn.Get(readPos);
    U currentValue = valuesPortalIn.Get(readPos);

    for (++readPos; readPos < numberOfKeys; ++readPos)
    {
      while (readPos < numberOfKeys && currentKey == keysPortalIn.Get(readPos))
      {
        currentValue = binary_functor(currentValue, valuesPortalIn.Get(readPos));
        ++readPos;
      }

      if (readPos < numberOfKeys)
      {
        keysPortalOut.Set(writePos, currentKey);
        valuesPortalOut.Set(writePos, currentValue);
        ++writePos;

        currentKey = keysPortalIn.Get(readPos);
        currentValue = valuesPortalIn.Get(readPos);
      }
    }

    //now write out the last set of values
    keysPortalOut.Set(writePos, currentKey);
    valuesPortalOut.Set(writePos, currentValue);

    //now we need to shrink to the correct number of keys/values
    //writePos is zero-based so add 1 to get correct length
    keys_output.Shrink(writePos + 1);
    values_output.Shrink(writePos + 1);
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binary_functor);

    svtkm::Id numberOfValues = input.GetNumberOfValues();

    auto inputPortal = input.PrepareForInput(Device());
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0)
    {
      return svtkm::TypeTraits<T>::ZeroInitialization();
    }

    std::partial_sum(svtkm::cont::ArrayPortalToIteratorBegin(inputPortal),
                     svtkm::cont::ArrayPortalToIteratorEnd(inputPortal),
                     svtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanInclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return ScanInclusive(input, output, svtkm::Sum());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binaryFunctor);

    svtkm::Id numberOfValues = input.GetNumberOfValues();

    auto inputPortal = input.PrepareForInput(Device());
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device());

    if (numberOfValues <= 0)
    {
      return initialValue;
    }

    // Shift right by one, by iterating backwards. We are required to iterate
    //backwards so that the algorithm works correctly when the input and output
    //are the same array, otherwise you just propagate the first element
    //to all elements
    //Note: We explicitly do not use std::copy_backwards for good reason.
    //The ICC compiler has been found to improperly optimize the copy_backwards
    //into a standard copy, causing the above issue.
    T lastValue = inputPortal.Get(numberOfValues - 1);
    for (svtkm::Id i = (numberOfValues - 1); i >= 1; --i)
    {
      outputPortal.Set(i, inputPortal.Get(i - 1));
    }
    outputPortal.Set(0, initialValue);

    std::partial_sum(svtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     svtkm::cont::ArrayPortalToIteratorEnd(outputPortal),
                     svtkm::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    return wrappedBinaryOp(outputPortal.Get(numberOfValues - 1), lastValue);
  }

  template <typename T, class CIn, class COut>
  SVTKM_CONT static T ScanExclusive(const svtkm::cont::ArrayHandle<T, CIn>& input,
                                   svtkm::cont::ArrayHandle<T, COut>& output)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    return ScanExclusive(input, output, svtkm::Sum(), svtkm::TypeTraits<T>::ZeroInitialization());
  }

  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::serial::internal::TaskTiling1D& functor,
                                            svtkm::Id size);
  SVTKM_CONT_EXPORT static void ScheduleTask(svtkm::exec::serial::internal::TaskTiling3D& functor,
                                            svtkm::Id3 size);

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id size)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::exec::serial::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, size);
  }

  template <class FunctorType>
  SVTKM_CONT static inline void Schedule(FunctorType functor, svtkm::Id3 size)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    svtkm::exec::serial::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, size);
  }

private:
  template <typename Vin,
            typename I,
            typename Vout,
            class StorageVin,
            class StorageI,
            class StorageVout>
  SVTKM_CONT static void Scatter(svtkm::cont::ArrayHandle<Vin, StorageVin>& values,
                                svtkm::cont::ArrayHandle<I, StorageI>& index,
                                svtkm::cont::ArrayHandle<Vout, StorageVout>& values_out)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    const svtkm::Id n = values.GetNumberOfValues();
    SVTKM_ASSERT(n == index.GetNumberOfValues());

    auto valuesPortal = values.PrepareForInput(Device());
    auto indexPortal = index.PrepareForInput(Device());
    auto valuesOutPortal = values_out.PrepareForOutput(n, Device());

    for (svtkm::Id i = 0; i < n; i++)
    {
      valuesOutPortal.Set(i, valuesPortal.Get(indexPortal.Get(i)));
    }
  }

private:
  /// Reorder the value array along with the sorting algorithm
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  SVTKM_CONT static void SortByKeyDirect(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                                        svtkm::cont::ArrayHandle<U, StorageU>& values,
                                        BinaryCompare binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using the custom compare
    //functor that the user passed in
    auto zipHandle = svtkm::cont::make_ArrayHandleZip(keys, values);
    Sort(zipHandle, internal::KeyCompare<T, U, BinaryCompare>(binary_compare));
  }

public:
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
                                  const BinaryCompare& binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    constexpr bool larger_than_64bits = sizeof(U) > sizeof(svtkm::Int64);
    if (larger_than_64bits)
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last
      svtkm::cont::ArrayHandle<svtkm::Id> indexArray;
      svtkm::cont::ArrayHandle<U, StorageU> valuesScattered;

      Copy(ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);
      SortByKeyDirect(keys, indexArray, wrappedCompare);
      Scatter(values, indexArray, valuesScattered);
      Copy(valuesScattered, values);
    }
    else
    {
      SortByKeyDirect(keys, values, wrappedCompare);
    }
  }

  template <typename T, class Storage>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    Sort(values, std::less<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  SVTKM_CONT static void Sort(svtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

    auto arrayPortal = values.PrepareForInPlace(Device());
    svtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    std::sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
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

    auto arrayPortal = values.PrepareForInPlace(Device());
    svtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);
    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);

    auto end = std::unique(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
    values.Shrink(static_cast<svtkm::Id>(end - iterators.GetBegin()));
  }

  SVTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device is serial and has no asynchronous operations.
  }
};

template <>
class DeviceTaskTypes<svtkm::cont::DeviceAdapterTagSerial>
{
public:
  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::serial::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                             InvocationType& invocation,
                                                             svtkm::Id,
                                                             svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::serial::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static svtkm::exec::serial::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                             InvocationType& invocation,
                                                             svtkm::Id3,
                                                             svtkm::Id globalIndexOffset = 0)
  {
    return svtkm::exec::serial::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace svtkm::cont

#endif //svtk_m_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
