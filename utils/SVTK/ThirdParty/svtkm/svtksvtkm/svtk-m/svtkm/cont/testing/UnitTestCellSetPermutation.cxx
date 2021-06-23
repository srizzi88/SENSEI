//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellSetPermutation.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace
{

struct WorkletPointToCell : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  SVTKM_EXEC void operator()(const PointIndicesType& pointIndices, svtkm::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint numCells);
  using ExecutionSignature = void(CellIndices, _2);
  using InputDomain = _1;

  template <typename CellIndicesType>
  SVTKM_EXEC void operator()(const CellIndicesType& cellIndices, svtkm::Id& numCells) const
  {
    numCells = cellIndices.GetNumberOfComponents();
  }
};

struct CellsOfPoint : public svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldInPoint offset, WholeArrayOut cellIds);
  using ExecutionSignature = void(CellIndices, _2, _3);
  using InputDomain = _1;

  template <typename CellIndicesType, typename CellIdsPortal>
  SVTKM_EXEC void operator()(const CellIndicesType& cellIndices,
                            svtkm::Id offset,
                            const CellIdsPortal& out) const
  {
    svtkm::IdComponent count = cellIndices.GetNumberOfComponents();
    for (svtkm::IdComponent i = 0; i < count; ++i)
    {
      out.Set(offset++, cellIndices[i]);
    }
  }
};

template <typename CellSetType, typename PermutationArrayHandleType>
std::vector<svtkm::Id> ComputeCellToPointExpected(const CellSetType& cellset,
                                                 const PermutationArrayHandleType& permutation)
{
  svtkm::cont::ArrayHandle<svtkm::Id> numIndices;
  svtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, numIndices);
  std::cout << "\n";

  svtkm::cont::ArrayHandle<svtkm::Id> indexOffsets;
  svtkm::Id connectivityLength = svtkm::cont::Algorithm::ScanExclusive(numIndices, indexOffsets);

  svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
  connectivity.Allocate(connectivityLength);
  svtkm::worklet::DispatcherMapTopology<CellsOfPoint>().Invoke(cellset, indexOffsets, connectivity);

  std::vector<bool> permutationMask(static_cast<std::size_t>(cellset.GetNumberOfCells()), false);
  for (svtkm::Id i = 0; i < permutation.GetNumberOfValues(); ++i)
  {
    permutationMask[static_cast<std::size_t>(permutation.GetPortalConstControl().Get(i))] = true;
  }

  svtkm::Id numberOfPoints = cellset.GetNumberOfPoints();
  std::vector<svtkm::Id> expected(static_cast<std::size_t>(numberOfPoints), 0);
  for (svtkm::Id i = 0; i < numberOfPoints; ++i)
  {
    svtkm::Id offset = indexOffsets.GetPortalConstControl().Get(i);
    svtkm::Id count = numIndices.GetPortalConstControl().Get(i);
    for (svtkm::Id j = 0; j < count; ++j)
    {
      svtkm::Id cellId = connectivity.GetPortalConstControl().Get(offset++);
      if (permutationMask[static_cast<std::size_t>(cellId)])
      {
        ++expected[static_cast<std::size_t>(i)];
      }
    }
  }

  return expected;
}

template <typename CellSetType>
svtkm::cont::CellSetPermutation<CellSetType, svtkm::cont::ArrayHandleCounting<svtkm::Id>> TestCellSet(
  const CellSetType& cellset)
{
  svtkm::Id numberOfCells = cellset.GetNumberOfCells() / 2;
  svtkm::cont::ArrayHandleCounting<svtkm::Id> permutation(0, 2, numberOfCells);
  auto cs = svtkm::cont::make_CellSetPermutation(permutation, cellset);
  svtkm::cont::ArrayHandle<svtkm::Id> result;

  std::cout << "\t\tTesting PointToCell\n";
  svtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cs, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == numberOfCells,
                   "result length not equal to number of cells");
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) ==
                       cellset.GetNumberOfPointsInCell(permutation.GetPortalConstControl().Get(i)),
                     "incorrect result");
  }

  std::cout << "\t\tTesting CellToPoint\n";
  svtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cs, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");
  auto expected = ComputeCellToPointExpected(cellset, permutation);
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == expected[static_cast<std::size_t>(i)],
                     "incorrect result");
  }
  std::cout << "Testing resource releasing in CellSetPermutation:\n";
  cs.ReleaseResourcesExecution();
  SVTKM_TEST_ASSERT(cs.GetNumberOfCells() == cellset.GetNumberOfCells() / 2,
                   "release execution resources should not change the number of cells");
  SVTKM_TEST_ASSERT(cs.GetNumberOfPoints() == cellset.GetNumberOfPoints(),
                   "release execution resources should not change the number of points");

  return cs;
}

template <typename CellSetType>
void RunTests(const CellSetType& cellset)
{
  std::cout << "\tTesting CellSetPermutation:\n";
  auto p1 = TestCellSet(cellset);
  std::cout << "\tTesting CellSetPermutation of CellSetPermutation:\n";
  TestCellSet(p1);
  std::cout << "----------------------------------------------------------\n";
}

void TestCellSetPermutation()
{
  svtkm::cont::DataSet dataset;
  svtkm::cont::testing::MakeTestDataSet maker;

  std::cout << "Testing CellSetStructured<2>\n";
  dataset = maker.Make2DUniformDataSet1();
  RunTests(dataset.GetCellSet().Cast<svtkm::cont::CellSetStructured<2>>());

  std::cout << "Testing CellSetStructured<3>\n";
  dataset = maker.Make3DUniformDataSet1();
  RunTests(dataset.GetCellSet().Cast<svtkm::cont::CellSetStructured<3>>());

  std::cout << "Testing CellSetExplicit\n";
  dataset = maker.Make3DExplicitDataSetPolygonal();
  RunTests(dataset.GetCellSet().Cast<svtkm::cont::CellSetExplicit<>>());

  std::cout << "Testing CellSetSingleType\n";
  dataset = maker.Make3DExplicitDataSetCowNose();
  RunTests(dataset.GetCellSet().Cast<svtkm::cont::CellSetSingleType<>>());
}

} // anonymous namespace

int UnitTestCellSetPermutation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellSetPermutation, argc, argv);
}
