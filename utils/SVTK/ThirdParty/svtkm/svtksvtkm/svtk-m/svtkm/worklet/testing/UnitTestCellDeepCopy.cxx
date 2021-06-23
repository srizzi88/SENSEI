//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/CellDeepCopy.h>

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetPermutation.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

svtkm::cont::CellSetExplicit<> CreateCellSet()
{
  svtkm::cont::testing::MakeTestDataSet makeData;
  svtkm::cont::DataSet data = makeData.Make3DExplicitDataSet0();
  svtkm::cont::CellSetExplicit<> cellSet;
  data.GetCellSet().CopyTo(cellSet);
  return cellSet;
}

svtkm::cont::CellSetPermutation<svtkm::cont::CellSetExplicit<>,
                               svtkm::cont::ArrayHandleCounting<svtkm::Id>>
CreatePermutedCellSet()
{
  std::cout << "Creating input cell set" << std::endl;

  svtkm::cont::CellSetExplicit<> cellSet = CreateCellSet();
  return svtkm::cont::make_CellSetPermutation(
    svtkm::cont::ArrayHandleCounting<svtkm::Id>(
      cellSet.GetNumberOfCells() - 1, -1, cellSet.GetNumberOfCells()),
    cellSet);
}

template <typename CellSetType>
svtkm::cont::CellSetExplicit<> DoCellDeepCopy(const CellSetType& inCells)
{
  std::cout << "Doing cell copy" << std::endl;

  return svtkm::worklet::CellDeepCopy::Run(inCells);
}

void CheckOutput(const svtkm::cont::CellSetExplicit<>& copiedCells)
{
  std::cout << "Checking copied cells" << std::endl;

  svtkm::cont::CellSetExplicit<> originalCells = CreateCellSet();

  svtkm::Id numberOfCells = copiedCells.GetNumberOfCells();
  SVTKM_TEST_ASSERT(numberOfCells == originalCells.GetNumberOfCells(),
                   "Result has wrong number of cells");

  // Cells should be copied backward. Check that.
  for (svtkm::Id cellIndex = 0; cellIndex < numberOfCells; cellIndex++)
  {
    svtkm::Id oCellIndex = numberOfCells - cellIndex - 1;
    SVTKM_TEST_ASSERT(copiedCells.GetCellShape(cellIndex) == originalCells.GetCellShape(oCellIndex),
                     "Bad cell shape");

    svtkm::IdComponent numPoints = copiedCells.GetNumberOfPointsInCell(cellIndex);
    SVTKM_TEST_ASSERT(numPoints == originalCells.GetNumberOfPointsInCell(oCellIndex),
                     "Bad number of points in cell");

    // Only checking 3 points. All cells should have at least 3
    svtkm::Id3 cellPoints{ 0 };
    copiedCells.GetIndices(cellIndex, cellPoints);
    svtkm::Id3 oCellPoints{ 0 };
    originalCells.GetIndices(oCellIndex, oCellPoints);
    SVTKM_TEST_ASSERT(cellPoints == oCellPoints, "Point indices not copied correctly");
  }
}

void RunTest()
{
  svtkm::cont::CellSetExplicit<> cellSet = DoCellDeepCopy(CreatePermutedCellSet());
  CheckOutput(cellSet);
}

} // anonymous namespace

int UnitTestCellDeepCopy(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
