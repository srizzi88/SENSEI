//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/connectivities/InnerJoin.h>


class TestInnerJoin
{
public:
  template <typename T, typename Storage>
  bool TestArrayHandle(const svtkm::cont::ArrayHandle<T, Storage>& ah,
                       const T* expected,
                       svtkm::Id size) const
  {
    if (size != ah.GetNumberOfValues())
    {
      return false;
    }

    for (svtkm::Id i = 0; i < size; ++i)
    {
      if (ah.GetPortalConstControl().Get(i) != expected[i])
      {
        return false;
      }
    }

    return true;
  }

  void TestTwoArrays() const
  {
    using Algorithm = svtkm::cont::Algorithm;

    std::vector<svtkm::Id> A = { 8, 3, 6, 8, 9, 5, 12, 10, 14 };
    std::vector<svtkm::Id> B = { 7, 11, 9, 8, 5, 1, 0, 5 };

    svtkm::cont::ArrayHandle<svtkm::Id> A_arr = svtkm::cont::make_ArrayHandle(A);
    svtkm::cont::ArrayHandle<svtkm::Id> B_arr = svtkm::cont::make_ArrayHandle(B);
    svtkm::cont::ArrayHandle<svtkm::Id> idxA;
    svtkm::cont::ArrayHandle<svtkm::Id> idxB;

    Algorithm::Copy(svtkm::cont::ArrayHandleIndex(A_arr.GetNumberOfValues()), idxA);
    Algorithm::Copy(svtkm::cont::ArrayHandleIndex(B_arr.GetNumberOfValues()), idxB);

    svtkm::cont::ArrayHandle<svtkm::Id> joinedIndex;
    svtkm::cont::ArrayHandle<svtkm::Id> outA;
    svtkm::cont::ArrayHandle<svtkm::Id> outB;

    svtkm::worklet::connectivity::InnerJoin().Run(A_arr, idxA, B_arr, idxB, joinedIndex, outA, outB);

    svtkm::Id expectedIndex[] = { 5, 5, 8, 8, 9 };
    SVTKM_TEST_ASSERT(TestArrayHandle(joinedIndex, expectedIndex, 5), "Wrong joined keys");

    svtkm::Id expectedOutA[] = { 5, 5, 0, 3, 4 };
    SVTKM_TEST_ASSERT(TestArrayHandle(outA, expectedOutA, 5), "Wrong joined values");

    svtkm::Id expectedOutB[] = { 4, 7, 3, 3, 2 };
    SVTKM_TEST_ASSERT(TestArrayHandle(outB, expectedOutB, 5), "Wrong joined values");
  }

  void operator()() const { this->TestTwoArrays(); }
};

int UnitTestInnerJoin(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestInnerJoin(), argc, argv);
}
