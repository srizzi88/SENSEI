//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/MaskSelect.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleView.h>

namespace
{

struct ReverseOutputToThreadMap : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputToThreadMap,
                                FieldIn maskArray,
                                WholeArrayOut threadToOutputMap);
  using ExecutionSignature = void(_1, InputIndex, _2, _3);

  template <typename MaskType, typename ThreadToOutputPortal>
  SVTKM_EXEC void operator()(svtkm::Id threadIndex,
                            svtkm::Id outputIndex,
                            MaskType mask,
                            ThreadToOutputPortal threadToOutput) const
  {
    if (mask)
    {
      threadToOutput.Set(threadIndex, outputIndex);
    }
  }
};

SVTKM_CONT static svtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapWithFind(
  svtkm::Id numThreads,
  svtkm::cont::ArrayHandle<svtkm::Id> outputToThreadMap,
  svtkm::cont::DeviceAdapterId device)
{
  svtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;

  svtkm::Id outputSize = outputToThreadMap.GetNumberOfValues();

  svtkm::cont::ArrayHandleIndex threadIndices(numThreads);
  svtkm::cont::Algorithm::UpperBounds(
    device,
    svtkm::cont::make_ArrayHandleView(outputToThreadMap, 1, outputSize - 1),
    threadIndices,
    threadToOutputMap);

  return threadToOutputMap;
}

template <typename MaskArrayType>
SVTKM_CONT static svtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapWithCopy(
  svtkm::Id numThreads,
  const svtkm::cont::ArrayHandle<svtkm::Id>& outputToThreadMap,
  const MaskArrayType& maskArray,
  svtkm::cont::DeviceAdapterId device)
{
  svtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);

  svtkm::worklet::DispatcherMapField<ReverseOutputToThreadMap> dispatcher;
  dispatcher.SetDevice(device);
  dispatcher.Invoke(outputToThreadMap, maskArray, threadToOutputMap);

  return threadToOutputMap;
}

SVTKM_CONT static svtkm::worklet::MaskSelect::ThreadToOutputMapType BuildThreadToOutputMapAllOn(
  svtkm::Id numThreads,
  svtkm::cont::DeviceAdapterId device)
{
  svtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  threadToOutputMap.Allocate(numThreads);
  svtkm::cont::Algorithm::Copy(
    device, svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(0, 1, numThreads), threadToOutputMap);
  return threadToOutputMap;
}

struct MaskBuilder
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& maskArray,
                  svtkm::worklet::MaskSelect::ThreadToOutputMapType& threadToOutputMap,
                  svtkm::cont::DeviceAdapterId device)
  {
    svtkm::cont::ArrayHandle<svtkm::Id> outputToThreadMap;
    svtkm::Id numThreads = svtkm::cont::Algorithm::ScanExclusive(
      device, svtkm::cont::make_ArrayHandleCast<svtkm::Id>(maskArray), outputToThreadMap);
    SVTKM_ASSERT(numThreads <= maskArray.GetNumberOfValues());

    // We have implemented two different ways to compute the thread to output map. The first way is
    // to use a binary search on each thread index into the output map. The second way is to
    // schedule on each output and copy the the index to the thread map. The first way is faster
    // for output sizes that are small relative to the input and also tends to be well load
    // balanced. The second way is faster for larger outputs.
    //
    // The former is obviously faster for one thread and the latter is obviously faster when all
    // outputs have a thread. We have to guess for values in the middle. I'm using if the square of
    // the number of threads is less than the number of outputs because it is easy to compute.
    if (numThreads == maskArray.GetNumberOfValues())
    { //fast path when everything is on
      threadToOutputMap = BuildThreadToOutputMapAllOn(numThreads, device);
    }
    else if ((numThreads * numThreads) < maskArray.GetNumberOfValues())
    {
      threadToOutputMap = BuildThreadToOutputMapWithFind(numThreads, outputToThreadMap, device);
    }
    else
    {
      threadToOutputMap =
        BuildThreadToOutputMapWithCopy(numThreads, outputToThreadMap, maskArray, device);
    }
  }
};

} // anonymous namespace

svtkm::worklet::MaskSelect::ThreadToOutputMapType svtkm::worklet::MaskSelect::Build(
  const VariantArrayHandleMask& maskArray,
  svtkm::cont::DeviceAdapterId device)
{
  svtkm::worklet::MaskSelect::ThreadToOutputMapType threadToOutputMap;
  maskArray.CastAndCall(MaskBuilder(), threadToOutputMap, device);
  return threadToOutputMap;
}
