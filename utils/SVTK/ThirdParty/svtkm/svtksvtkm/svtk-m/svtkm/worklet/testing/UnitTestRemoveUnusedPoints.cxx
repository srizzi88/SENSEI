//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/RemoveUnusedPoints.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

svtkm::cont::CellSetExplicit<> CreateInputCellSet()
{
  svtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(svtkm::CELL_SHAPE_TRIANGLE, 3, svtkm::make_Vec<svtkm::Id>(0, 2, 4));
  cellSet.AddCell(svtkm::CELL_SHAPE_QUAD, 4, svtkm::make_Vec<svtkm::Id>(4, 2, 6, 8));
  cellSet.CompleteAddingCells(11);
  return cellSet;
}

void CheckOutputCellSet(const svtkm::cont::CellSetExplicit<>& cellSet,
                        const svtkm::cont::ArrayHandle<svtkm::Float32>& field)
{
  SVTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong num cells.");
  SVTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 5, "Wrong num points.");

  SVTKM_TEST_ASSERT(cellSet.GetCellShape(0) == svtkm::CELL_SHAPE_TRIANGLE, "Wrong shape");
  SVTKM_TEST_ASSERT(cellSet.GetCellShape(1) == svtkm::CELL_SHAPE_QUAD, "Wrong shape");

  SVTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(0) == 3, "Wrong num points");
  SVTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(1) == 4, "Wrong num points");

  svtkm::Id3 pointIds3;
  cellSet.GetIndices(0, pointIds3);
  SVTKM_TEST_ASSERT(pointIds3[0] == 0, "Wrong point id for cell");
  SVTKM_TEST_ASSERT(pointIds3[1] == 1, "Wrong point id for cell");
  SVTKM_TEST_ASSERT(pointIds3[2] == 2, "Wrong point id for cell");

  svtkm::Id4 pointIds4;
  cellSet.GetIndices(1, pointIds4);
  SVTKM_TEST_ASSERT(pointIds4[0] == 2, "Wrong point id for cell");
  SVTKM_TEST_ASSERT(pointIds4[1] == 1, "Wrong point id for cell");
  SVTKM_TEST_ASSERT(pointIds4[2] == 3, "Wrong point id for cell");
  SVTKM_TEST_ASSERT(pointIds4[3] == 4, "Wrong point id for cell");

  auto fieldPortal = field.GetPortalConstControl();
  SVTKM_TEST_ASSERT(test_equal(fieldPortal.Get(0), TestValue(0, svtkm::Float32())), "Bad field");
  SVTKM_TEST_ASSERT(test_equal(fieldPortal.Get(1), TestValue(2, svtkm::Float32())), "Bad field");
  SVTKM_TEST_ASSERT(test_equal(fieldPortal.Get(2), TestValue(4, svtkm::Float32())), "Bad field");
  SVTKM_TEST_ASSERT(test_equal(fieldPortal.Get(3), TestValue(6, svtkm::Float32())), "Bad field");
  SVTKM_TEST_ASSERT(test_equal(fieldPortal.Get(4), TestValue(8, svtkm::Float32())), "Bad field");
}

void RunTest()
{
  std::cout << "Creating input" << std::endl;
  svtkm::cont::CellSetExplicit<> inCellSet = CreateInputCellSet();

  svtkm::cont::ArrayHandle<svtkm::Float32> inField;
  inField.Allocate(inCellSet.GetNumberOfPoints());
  SetPortal(inField.GetPortalControl());

  std::cout << "Removing unused points" << std::endl;
  svtkm::worklet::RemoveUnusedPoints compactPoints(inCellSet);
  svtkm::cont::CellSetExplicit<> outCellSet = compactPoints.MapCellSet(inCellSet);
  svtkm::cont::ArrayHandle<svtkm::Float32> outField = compactPoints.MapPointFieldDeep(inField);

  std::cout << "Checking resulting cell set" << std::endl;
  CheckOutputCellSet(outCellSet, outField);
}

} // anonymous namespace

int UnitTestRemoveUnusedPoints(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
