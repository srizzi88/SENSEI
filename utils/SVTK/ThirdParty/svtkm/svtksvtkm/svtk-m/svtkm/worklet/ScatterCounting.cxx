//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/ScatterCounting.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleView.h>
#include <svtkm/cont/ErrorBadValue.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <sstream>

namespace
{

SVTKM_CONT
inline svtkm::cont::ArrayHandleConcatenate<
  svtkm::cont::ArrayHandleConstant<svtkm::Id>,
  svtkm::cont::ArrayHandleView<svtkm::cont::ArrayHandle<svtkm::Id>>>
ShiftArrayHandleByOne(const svtkm::cont::ArrayHandle<svtkm::Id>& array)
{
  return svtkm::cont::make_ArrayHandleConcatenate(
    svtkm::cont::make_ArrayHandleConstant<svtkm::Id>(0, 1),
    svtkm::cont::make_ArrayHandleView(array, 0, array.GetNumberOfValues() - 1));
}

struct ReverseInputToOutputMapWorklet : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outputStartIndices,
                                FieldIn outputEndIndices,
                                WholeArrayOut outputToInputMap,
                                WholeArrayOut visit);
  using ExecutionSignature = void(_1, _2, _3, _4, InputIndex);
  using InputDomain = _2;

  template <typename OutputMapType, typename VisitType>
  SVTKM_EXEC void operator()(svtkm::Id outputStartIndex,
                            svtkm::Id outputEndIndex,
                            const OutputMapType& outputToInputMap,
                            const VisitType& visit,
                            svtkm::Id inputIndex) const
  {
    svtkm::IdComponent visitIndex = 0;
    for (svtkm::Id outputIndex = outputStartIndex; outputIndex < outputEndIndex; outputIndex++)
    {
      outputToInputMap.Set(outputIndex, inputIndex);
      visit.Set(outputIndex, visitIndex);
      visitIndex++;
    }
  }

  SVTKM_CONT
  static void Run(const svtkm::cont::ArrayHandle<svtkm::Id>& inputToOutputMap,
                  const svtkm::cont::ArrayHandle<svtkm::Id>& outputToInputMap,
                  const svtkm::cont::ArrayHandle<svtkm::IdComponent>& visit,
                  svtkm::cont::DeviceAdapterId device)
  {
    svtkm::worklet::DispatcherMapField<ReverseInputToOutputMapWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(
      ShiftArrayHandleByOne(inputToOutputMap), inputToOutputMap, outputToInputMap, visit);
  }
};

struct SubtractToVisitIndexWorklet : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn startsOfGroup, WholeArrayOut visit);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  template <typename VisitType>
  SVTKM_EXEC void operator()(svtkm::Id inputIndex,
                            svtkm::Id startOfGroup,
                            const VisitType& visit) const
  {
    svtkm::IdComponent visitIndex = static_cast<svtkm::IdComponent>(inputIndex - startOfGroup);
    visit.Set(inputIndex, visitIndex);
  }
};

} // anonymous namespace

namespace svtkm
{
namespace worklet
{
namespace detail
{

struct ScatterCountingBuilder
{
  template <typename CountArrayType>
  SVTKM_CONT static void BuildArrays(svtkm::worklet::ScatterCounting* self,
                                    const CountArrayType& countArray,
                                    svtkm::cont::DeviceAdapterId device,
                                    bool saveInputToOutputMap)
  {
    SVTKM_IS_ARRAY_HANDLE(CountArrayType);

    self->InputRange = countArray.GetNumberOfValues();

    // The input to output map is actually built off by one. The first entry
    // is actually for the second value. The last entry is the total number of
    // output. This off-by-one is so that an upper bound find will work when
    // building the output to input map. Later we will either correct the
    // map or delete it.
    svtkm::cont::ArrayHandle<svtkm::Id> inputToOutputMapOffByOne;
    svtkm::Id outputSize = svtkm::cont::Algorithm::ScanInclusive(
      device, svtkm::cont::make_ArrayHandleCast(countArray, svtkm::Id()), inputToOutputMapOffByOne);

    // We have implemented two different ways to compute the output to input
    // map. The first way is to use a binary search on each output index into
    // the input map. The second way is to schedule on each input and
    // iteratively fill all the output indices for that input. The first way is
    // faster for output sizes that are small relative to the input (typical in
    // Marching Cubes, for example) and also tends to be well load balanced.
    // The second way is faster for larger outputs (typical in triangulation,
    // for example). We will use the first method for small output sizes and
    // the second for large output sizes. Toying with this might be a good
    // place for optimization.
    if (outputSize < self->InputRange)
    {
      BuildOutputToInputMapWithFind(self, outputSize, device, inputToOutputMapOffByOne);
    }
    else
    {
      BuildOutputToInputMapWithIterate(self, outputSize, device, inputToOutputMapOffByOne);
    }

    if (saveInputToOutputMap)
    {
      // Since we are saving it, correct the input to output map.
      svtkm::cont::Algorithm::Copy(
        device, ShiftArrayHandleByOne(inputToOutputMapOffByOne), self->InputToOutputMap);
    }
  }

  SVTKM_CONT static void BuildOutputToInputMapWithFind(
    svtkm::worklet::ScatterCounting* self,
    svtkm::Id outputSize,
    svtkm::cont::DeviceAdapterId device,
    svtkm::cont::ArrayHandle<svtkm::Id> inputToOutputMapOffByOne)
  {
    svtkm::cont::ArrayHandleIndex outputIndices(outputSize);
    svtkm::cont::Algorithm::UpperBounds(
      device, inputToOutputMapOffByOne, outputIndices, self->OutputToInputMap);

    svtkm::cont::ArrayHandle<svtkm::Id> startsOfGroups;

    // This find gives the index of the start of a group.
    svtkm::cont::Algorithm::LowerBounds(
      device, self->OutputToInputMap, self->OutputToInputMap, startsOfGroups);

    self->VisitArray.Allocate(outputSize);
    svtkm::worklet::DispatcherMapField<SubtractToVisitIndexWorklet> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(startsOfGroups, self->VisitArray);
  }

  SVTKM_CONT static void BuildOutputToInputMapWithIterate(
    svtkm::worklet::ScatterCounting* self,
    svtkm::Id outputSize,
    svtkm::cont::DeviceAdapterId device,
    svtkm::cont::ArrayHandle<svtkm::Id> inputToOutputMapOffByOne)
  {
    self->OutputToInputMap.Allocate(outputSize);
    self->VisitArray.Allocate(outputSize);

    ReverseInputToOutputMapWorklet::Run(
      inputToOutputMapOffByOne, self->OutputToInputMap, self->VisitArray, device);
  }

  template <typename ArrayType>
  void operator()(const ArrayType& countArray,
                  svtkm::cont::DeviceAdapterId device,
                  bool saveInputToOutputMap,
                  svtkm::worklet::ScatterCounting* self)
  {
    BuildArrays(self, countArray, device, saveInputToOutputMap);
  }
};
}
}
} // namespace svtkm::worklet::detail

void svtkm::worklet::ScatterCounting::BuildArrays(const VariantArrayHandleCount& countArray,
                                                 svtkm::cont::DeviceAdapterId device,
                                                 bool saveInputToOutputMap)
{
  countArray.CastAndCall(
    svtkm::worklet::detail::ScatterCountingBuilder(), device, saveInputToOutputMap, this);
}
