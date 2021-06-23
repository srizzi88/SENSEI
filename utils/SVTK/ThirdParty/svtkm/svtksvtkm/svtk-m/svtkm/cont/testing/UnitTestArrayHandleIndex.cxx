//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleIndex.h>

#include <svtkm/cont/testing/Testing.h>

namespace UnitTestArrayHandleIndexNamespace
{

const svtkm::Id ARRAY_SIZE = 10;

void TestArrayHandleIndex()
{
  svtkm::cont::ArrayHandleIndex array(ARRAY_SIZE);
  SVTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Bad size.");

  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    SVTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == index,
                     "Index array has unexpected value.");
  }
}

} // namespace UnitTestArrayHandleIndexNamespace

int UnitTestArrayHandleIndex(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleIndexNamespace;
  return svtkm::cont::testing::Testing::Run(TestArrayHandleIndex, argc, argv);
}
