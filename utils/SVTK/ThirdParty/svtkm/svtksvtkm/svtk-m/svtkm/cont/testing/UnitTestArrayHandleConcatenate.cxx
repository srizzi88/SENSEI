//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleIndex.h>

#include <svtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleConcatenateNamespace
{

const svtkm::Id ARRAY_SIZE = 5;

void TestArrayHandleConcatenate()
{
  svtkm::cont::ArrayHandleIndex array1(ARRAY_SIZE);
  svtkm::cont::ArrayHandleIndex array2(2 * ARRAY_SIZE);

  svtkm::cont::ArrayHandleConcatenate<svtkm::cont::ArrayHandleIndex, svtkm::cont::ArrayHandleIndex>
    array3(array1, array2);

  svtkm::cont::ArrayHandleIndex array4(ARRAY_SIZE);
  svtkm::cont::ArrayHandleConcatenate<
    svtkm::cont::ArrayHandleConcatenate<svtkm::cont::ArrayHandleIndex,  // 1st
                                       svtkm::cont::ArrayHandleIndex>, // ArrayHandle
    svtkm::cont::ArrayHandleIndex>                                     // 2nd ArrayHandle
    array5;
  {
    array5 = svtkm::cont::make_ArrayHandleConcatenate(array3, array4);
  }

  for (svtkm::Id index = 0; index < array5.GetNumberOfValues(); index++)
  {
    std::cout << array5.GetPortalConstControl().Get(index) << std::endl;
  }
}

void TestConcatenateEmptyArray()
{
  std::vector<svtkm::Float64> vec;
  for (svtkm::Id i = 0; i < ARRAY_SIZE; i++)
    vec.push_back(svtkm::Float64(i) * 1.5);

  using CoeffValueType = svtkm::Float64;
  using CoeffArrayTypeTmp = svtkm::cont::ArrayHandle<CoeffValueType>;
  using ArrayConcat = svtkm::cont::ArrayHandleConcatenate<CoeffArrayTypeTmp, CoeffArrayTypeTmp>;
  using ArrayConcat2 = svtkm::cont::ArrayHandleConcatenate<ArrayConcat, CoeffArrayTypeTmp>;

  CoeffArrayTypeTmp arr1 = svtkm::cont::make_ArrayHandle(vec);
  CoeffArrayTypeTmp arr2, arr3;

  ArrayConcat arrConc(arr2, arr1);
  ArrayConcat2 arrConc2(arrConc, arr3);

  for (svtkm::Id i = 0; i < arrConc2.GetNumberOfValues(); i++)
    std::cout << arrConc2.GetPortalConstControl().Get(i) << std::endl;
}

} // namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleConcatenate(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleConcatenateNamespace;
  //TestConcatenateEmptyArray();
  return svtkm::cont::testing::Testing::Run(TestArrayHandleConcatenate, argc, argv);
}
