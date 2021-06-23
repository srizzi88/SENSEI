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

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

constexpr svtkm::Id nullValue = -2;

struct TestMaskArrays
{
  svtkm::cont::ArrayHandle<svtkm::IdComponent> SelectArray;
  svtkm::cont::ArrayHandle<svtkm::Id> ThreadToOutputMap;
};

TestMaskArrays MakeMaskArraysShort()
{
  const svtkm::Id selectArraySize = 18;
  const svtkm::IdComponent selectArray[selectArraySize] = { 1, 1, 0, 0, 0, 0, 1, 0, 0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 1 };
  const svtkm::Id threadRange = 4;
  const svtkm::Id threadToOutputMap[threadRange] = { 0, 1, 6, 17 };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandle(threadToOutputMap, threadRange),
                        arrays.ThreadToOutputMap);

  return arrays;
}

TestMaskArrays MakeMaskArraysLong()
{
  const svtkm::Id selectArraySize = 16;
  const svtkm::IdComponent selectArray[selectArraySize] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
  };
  const svtkm::Id threadRange = 15;
  const svtkm::Id threadToOutputMap[threadRange] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15
  };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandle(threadToOutputMap, threadRange),
                        arrays.ThreadToOutputMap);

  return arrays;
}

TestMaskArrays MakeMaskArraysZero()
{
  const svtkm::Id selectArraySize = 6;
  const svtkm::IdComponent selectArray[selectArraySize] = { 0, 0, 0, 0, 0, 0 };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  arrays.ThreadToOutputMap.Allocate(0);

  return arrays;
}

struct TestMaskSelectWorklet : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputIndices, FieldInOut copyIndices);
  using ExecutionSignature = void(_1, _2);

  using MaskType = svtkm::worklet::MaskSelect;

  SVTKM_EXEC
  void operator()(svtkm::Id inputIndex, svtkm::Id& indexCopy) const { indexCopy = inputIndex; }
};

template <typename T, typename SelectArrayType>
void CompareArrays(svtkm::cont::ArrayHandle<T> array1,
                   svtkm::cont::ArrayHandle<T> array2,
                   const SelectArrayType& selectArray)
{
  SVTKM_IS_ARRAY_HANDLE(SelectArrayType);

  auto portal1 = array1.GetPortalConstControl();
  auto portal2 = array2.GetPortalConstControl();
  auto selectPortal = selectArray.GetPortalConstControl();

  SVTKM_TEST_ASSERT(portal1.GetNumberOfValues() == portal2.GetNumberOfValues());
  SVTKM_TEST_ASSERT(portal1.GetNumberOfValues() == selectArray.GetNumberOfValues());

  for (svtkm::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (selectPortal.Get(index))
    {
      T value1 = portal1.Get(index);
      T value2 = portal2.Get(index);
      SVTKM_TEST_ASSERT(
        value1 == value2, "Array values not equal (", index, ": ", value1, " ", value2, ")");
    }
    else
    {
      T value = portal2.Get(index);
      SVTKM_TEST_ASSERT(value == nullValue, "Expected null value, got ", value);
    }
  }
}

template <typename T>
void CompareArrays(svtkm::cont::ArrayHandle<T> array1, svtkm::cont::ArrayHandle<T> array2)
{
  CompareArrays(
    array1, array2, svtkm::cont::make_ArrayHandleConstant<bool>(true, array1.GetNumberOfValues()));
}

// This unit test makes sure the ScatterCounting generates the correct map
// and visit arrays.
void TestMaskArrayGeneration(const TestMaskArrays& arrays)
{
  std::cout << "  Testing array generation" << std::endl;

  svtkm::worklet::MaskSelect mask(arrays.SelectArray, svtkm::cont::DeviceAdapterTagAny());

  svtkm::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  std::cout << "    Checking thread to output map ";
  svtkm::cont::printSummary_ArrayHandle(mask.GetThreadToOutputMap(inputSize), std::cout);
  CompareArrays(arrays.ThreadToOutputMap, mask.GetThreadToOutputMap(inputSize));
}

// This is more of an integration test that makes sure the scatter works with a
// worklet invocation.
void TestMaskWorklet(const TestMaskArrays& arrays)
{
  std::cout << "  Testing mask select in a worklet." << std::endl;

  svtkm::worklet::DispatcherMapField<TestMaskSelectWorklet> dispatcher(
    svtkm::worklet::MaskSelect(arrays.SelectArray));

  svtkm::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  svtkm::cont::ArrayHandle<svtkm::Id> inputIndices;
  svtkm::cont::ArrayCopy(svtkm::cont::ArrayHandleIndex(inputSize), inputIndices);

  svtkm::cont::ArrayHandle<svtkm::Id> selectIndexCopy;
  svtkm::cont::ArrayCopy(svtkm::cont::ArrayHandleConstant<svtkm::Id>(nullValue, inputSize),
                        selectIndexCopy);

  std::cout << "    Invoke worklet" << std::endl;
  dispatcher.Invoke(inputIndices, selectIndexCopy);

  std::cout << "    Check copied indices." << std::endl;
  CompareArrays(inputIndices, selectIndexCopy, arrays.SelectArray);
}

void TestMaskSelectWithArrays(const TestMaskArrays& arrays)
{
  TestMaskArrayGeneration(arrays);
  TestMaskWorklet(arrays);
}

void TestMaskSelect()
{
  std::cout << "Testing arrays with output smaller than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysShort());

  std::cout << "Testing arrays with output larger than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysLong());

  std::cout << "Testing arrays with zero output." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysZero());
}

} // anonymous namespace

int UnitTestMaskSelect(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestMaskSelect, argc, argv);
}
