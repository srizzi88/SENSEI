//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleReverse.h>

#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleReverseNamespace
{

const svtkm::Id ARRAY_SIZE = 10;

void TestArrayHandleReverseRead()
{
  svtkm::cont::ArrayHandleIndex array(ARRAY_SIZE);
  SVTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");

  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    SVTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == index,
                     "Index array has unexpected value.");
  }

  svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandleIndex> reverse =
    svtkm::cont::make_ArrayHandleReverse(array);

  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    SVTKM_TEST_ASSERT(reverse.GetPortalConstControl().Get(index) ==
                       array.GetPortalConstControl().Get(9 - index),
                     "ArrayHandleReverse does not reverse array");
  }
}

void TestArrayHandleReverseWrite()
{
  std::vector<svtkm::Id> ids(ARRAY_SIZE, 0);
  svtkm::cont::ArrayHandle<svtkm::Id> handle = svtkm::cont::make_ArrayHandle(ids);

  svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<svtkm::Id>> reverse =
    svtkm::cont::make_ArrayHandleReverse(handle);

  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    reverse.GetPortalControl().Set(index, index);
  }

  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    SVTKM_TEST_ASSERT(handle.GetPortalConstControl().Get(index) == (9 - index),
                     "ArrayHandleReverse does not reverse array");
  }
}

void TestArrayHandleReverseScanInclusiveByKey()
{
  svtkm::Id ids[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  svtkm::Id seg[] = { 0, 0, 0, 0, 1, 1, 2, 3, 3, 4 };
  svtkm::cont::ArrayHandle<svtkm::Id> values = svtkm::cont::make_ArrayHandle(ids, 10);
  svtkm::cont::ArrayHandle<svtkm::Id> keys = svtkm::cont::make_ArrayHandle(seg, 10);

  svtkm::cont::ArrayHandle<svtkm::Id> output;
  svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<svtkm::Id>> reversed =
    svtkm::cont::make_ArrayHandleReverse(output);

  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<svtkm::cont::DeviceAdapterTagSerial>;
  Algorithm::ScanInclusiveByKey(keys, values, reversed);

  svtkm::Id expected[] = { 0, 1, 3, 6, 4, 9, 6, 7, 15, 9 };
  svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<svtkm::Id>> expected_reversed =
    svtkm::cont::make_ArrayHandleReverse(svtkm::cont::make_ArrayHandle(expected, 10));
  for (int i = 0; i < 10; i++)
  {
    SVTKM_TEST_ASSERT(output.GetPortalConstControl().Get(i) ==
                       expected_reversed.GetPortalConstControl().Get(i),
                     "ArrayHandleReverse as output of ScanInclusiveByKey");
  }
  std::cout << std::endl;
}

void TestArrayHandleReverse()
{
  TestArrayHandleReverseRead();
  TestArrayHandleReverseWrite();
  TestArrayHandleReverseScanInclusiveByKey();
}

}; // namespace UnitTestArrayHandleReverseNamespace

int UnitTestArrayHandleReverse(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleReverseNamespace;
  return svtkm::cont::testing::Testing::Run(TestArrayHandleReverse, argc, argv);
}
