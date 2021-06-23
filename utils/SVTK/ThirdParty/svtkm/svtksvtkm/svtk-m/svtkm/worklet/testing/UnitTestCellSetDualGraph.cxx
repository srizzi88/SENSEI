//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/Contour.h>

#include <svtkm/worklet/connectivities/CellSetDualGraph.h>

class TestCellSetDualGraph
{
private:
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

public:
  void TestTriangleMesh() const
  {
    std::vector<svtkm::Id> connectivity = { 0, 2, 4, 1, 3, 5, 2, 6, 4, 5, 3, 7, 2, 9, 6, 4, 6, 8 };

    svtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(8, svtkm::CELL_SHAPE_TRIANGLE, 3, svtkm::cont::make_ArrayHandle(connectivity));

    svtkm::cont::ArrayHandle<svtkm::Id> numIndicesArray;
    svtkm::cont::ArrayHandle<svtkm::Id> indexOffsetArray;
    svtkm::cont::ArrayHandle<svtkm::Id> connectivityArray;

    svtkm::worklet::connectivity::CellSetDualGraph().Run(
      cellSet, numIndicesArray, indexOffsetArray, connectivityArray);

    svtkm::Id expectedNumIndices[] = { 1, 1, 3, 1, 1, 1 };
    SVTKM_TEST_ASSERT(numIndicesArray.GetNumberOfValues() == 6,
                     "Wrong number of elements in NumIndices array");
    SVTKM_TEST_ASSERT(TestArrayHandle(numIndicesArray, expectedNumIndices, 6),
                     "Got incorrect numIndices");

    svtkm::Id expectedIndexOffset[] = { 0, 1, 2, 5, 6, 7 };
    SVTKM_TEST_ASSERT(TestArrayHandle(indexOffsetArray, expectedIndexOffset, 6),
                     "Got incorrect indexOffset");

    svtkm::Id expectedConnectivity[] = { 2, 3, 0, 4, 5, 1, 2, 2 };
    SVTKM_TEST_ASSERT(TestArrayHandle(connectivityArray, expectedConnectivity, 8),
                     "Got incorrect dual graph connectivity");
  }

  void operator()() const { this->TestTriangleMesh(); }
};

int UnitTestCellSetDualGraph(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellSetDualGraph(), argc, argv);
}
