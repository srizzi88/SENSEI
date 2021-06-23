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

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

struct TestScatterArrays
{
  svtkm::cont::ArrayHandle<svtkm::IdComponent> CountArray;
  svtkm::cont::ArrayHandle<svtkm::Id> InputToOutputMap;
  svtkm::cont::ArrayHandle<svtkm::Id> OutputToInputMap;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> VisitArray;
};

TestScatterArrays MakeScatterArraysShort()
{
  const svtkm::Id countArraySize = 18;
  const svtkm::IdComponent countArray[countArraySize] = { 1, 2, 0, 0, 1, 0, 1, 0, 0,
                                                         0, 0, 0, 0, 0, 1, 0, 0, 0 };
  const svtkm::Id inputToOutputMap[countArraySize] = { 0, 1, 3, 3, 3, 4, 4, 5, 5,
                                                      5, 5, 5, 5, 5, 5, 6, 6, 6 };
  const svtkm::Id outputSize = 6;
  const svtkm::Id outputToInputMap[outputSize] = { 0, 1, 1, 4, 6, 14 };
  const svtkm::IdComponent visitArray[outputSize] = { 0, 0, 1, 0, 0, 0 };

  TestScatterArrays arrays;
  using Algorithm = svtkm::cont::Algorithm;

  // Need to copy arrays so that the data does not go out of scope.
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(countArray, countArraySize), arrays.CountArray);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(inputToOutputMap, countArraySize),
                  arrays.InputToOutputMap);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(outputToInputMap, outputSize),
                  arrays.OutputToInputMap);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(visitArray, outputSize), arrays.VisitArray);

  return arrays;
}

TestScatterArrays MakeScatterArraysLong()
{
  const svtkm::Id countArraySize = 6;
  const svtkm::IdComponent countArray[countArraySize] = { 0, 1, 2, 3, 4, 5 };
  const svtkm::Id inputToOutputMap[countArraySize] = { 0, 0, 1, 3, 6, 10 };
  const svtkm::Id outputSize = 15;
  const svtkm::Id outputToInputMap[outputSize] = { 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5 };
  const svtkm::IdComponent visitArray[outputSize] = { 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4 };

  TestScatterArrays arrays;
  using Algorithm = svtkm::cont::Algorithm;

  // Need to copy arrays so that the data does not go out of scope.
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(countArray, countArraySize), arrays.CountArray);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(inputToOutputMap, countArraySize),
                  arrays.InputToOutputMap);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(outputToInputMap, outputSize),
                  arrays.OutputToInputMap);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(visitArray, outputSize), arrays.VisitArray);

  return arrays;
}

TestScatterArrays MakeScatterArraysZero()
{
  const svtkm::Id countArraySize = 6;
  const svtkm::IdComponent countArray[countArraySize] = { 0, 0, 0, 0, 0, 0 };
  const svtkm::Id inputToOutputMap[countArraySize] = { 0, 0, 0, 0, 0, 0 };

  TestScatterArrays arrays;
  using Algorithm = svtkm::cont::Algorithm;

  // Need to copy arrays so that the data does not go out of scope.
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(countArray, countArraySize), arrays.CountArray);
  Algorithm::Copy(svtkm::cont::make_ArrayHandle(inputToOutputMap, countArraySize),
                  arrays.InputToOutputMap);
  arrays.OutputToInputMap.Allocate(0);
  arrays.VisitArray.Allocate(0);

  return arrays;
}

struct TestScatterCountingWorklet : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputIndices,
                                FieldOut copyIndices,
                                FieldOut recordVisit,
                                FieldOut recordWorkId);
  using ExecutionSignature = void(_1, _2, _3, _4, VisitIndex, WorkIndex);

  using ScatterType = svtkm::worklet::ScatterCounting;

  template <typename CountArrayType>
  SVTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
  {
    return ScatterType(countArray);
  }

  SVTKM_EXEC
  void operator()(svtkm::Id inputIndex,
                  svtkm::Id& indexCopy,
                  svtkm::IdComponent& writeVisit,
                  svtkm::Float32& captureWorkId,
                  svtkm::IdComponent visitIndex,
                  svtkm::Id workId) const
  {
    indexCopy = inputIndex;
    writeVisit = visitIndex;
    captureWorkId = TestValue(workId, svtkm::Float32());
  }
};

template <typename T>
void CompareArrays(svtkm::cont::ArrayHandle<T> array1, svtkm::cont::ArrayHandle<T> array2)
{
  using PortalType = typename svtkm::cont::ArrayHandle<T>::PortalConstControl;
  PortalType portal1 = array1.GetPortalConstControl();
  PortalType portal2 = array2.GetPortalConstControl();

  SVTKM_TEST_ASSERT(portal1.GetNumberOfValues() == portal2.GetNumberOfValues(),
                   "Arrays are not the same length.");

  for (svtkm::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    T value1 = portal1.Get(index);
    T value2 = portal2.Get(index);
    SVTKM_TEST_ASSERT(value1 == value2, "Array values not equal.");
  }
}

// This unit test makes sure the ScatterCounting generates the correct map
// and visit arrays.
void TestScatterArrayGeneration(const TestScatterArrays& arrays)
{
  std::cout << "  Testing array generation" << std::endl;

  svtkm::worklet::ScatterCounting scatter(
    arrays.CountArray, svtkm::cont::DeviceAdapterTagAny(), true);

  svtkm::Id inputSize = arrays.CountArray.GetNumberOfValues();

  std::cout << "    Checking input to output map." << std::endl;
  CompareArrays(arrays.InputToOutputMap, scatter.GetInputToOutputMap());

  std::cout << "    Checking output to input map." << std::endl;
  CompareArrays(arrays.OutputToInputMap, scatter.GetOutputToInputMap(inputSize));

  std::cout << "    Checking visit array." << std::endl;
  CompareArrays(arrays.VisitArray, scatter.GetVisitArray(inputSize));
}

// This is more of an integration test that makes sure the scatter works with a
// worklet invocation.
void TestScatterWorklet(const TestScatterArrays& arrays)
{
  std::cout << "  Testing scatter counting in a worklet." << std::endl;

  svtkm::worklet::DispatcherMapField<TestScatterCountingWorklet> dispatcher(
    TestScatterCountingWorklet::MakeScatter(arrays.CountArray));

  svtkm::Id inputSize = arrays.CountArray.GetNumberOfValues();
  svtkm::cont::ArrayHandleIndex inputIndices(inputSize);
  svtkm::cont::ArrayHandle<svtkm::Id> outputToInputMapCopy;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> visitCopy;
  svtkm::cont::ArrayHandle<svtkm::Float32> captureWorkId;

  std::cout << "    Invoke worklet" << std::endl;
  dispatcher.Invoke(inputIndices, outputToInputMapCopy, visitCopy, captureWorkId);

  std::cout << "    Check output to input map." << std::endl;
  CompareArrays(outputToInputMapCopy, arrays.OutputToInputMap);
  std::cout << "    Check visit." << std::endl;
  CompareArrays(visitCopy, arrays.VisitArray);
  std::cout << "    Check work id." << std::endl;
  CheckPortal(captureWorkId.GetPortalConstControl());
}

void TestScatterCountingWithArrays(const TestScatterArrays& arrays)
{
  TestScatterArrayGeneration(arrays);
  TestScatterWorklet(arrays);
}

void TestScatterCounting()
{
  std::cout << "Testing arrays with output smaller than input." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysShort());

  std::cout << "Testing arrays with output larger than input." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysLong());

  std::cout << "Testing arrays with zero output." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysZero());
}

} // anonymous namespace

int UnitTestScatterCounting(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestScatterCounting, argc, argv);
}
