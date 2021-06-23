//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/CleanGrid.h>

#include <svtkm/filter/Contour.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestUniformGrid(svtkm::filter::CleanGrid clean)
{
  std::cout << "Testing 'clean' uniform grid." << std::endl;

  svtkm::cont::testing::MakeTestDataSet makeData;

  svtkm::cont::DataSet inData = makeData.Make2DUniformDataSet0();

  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  svtkm::cont::DataSet outData = clean.Execute(inData);
  SVTKM_TEST_ASSERT(outData.HasField("pointvar"), "Failed to map point field");
  SVTKM_TEST_ASSERT(outData.HasField("cellvar"), "Failed to map cell field");

  svtkm::cont::CellSetExplicit<> outCellSet;
  outData.GetCellSet().CopyTo(outCellSet);
  SVTKM_TEST_ASSERT(outCellSet.GetNumberOfPoints() == 6,
                   "Wrong number of points: ",
                   outCellSet.GetNumberOfPoints());
  SVTKM_TEST_ASSERT(
    outCellSet.GetNumberOfCells() == 2, "Wrong number of cells: ", outCellSet.GetNumberOfCells());
  svtkm::Id4 cellIds;
  outCellSet.GetIndices(0, cellIds);
  SVTKM_TEST_ASSERT((cellIds == svtkm::Id4(0, 1, 4, 3)), "Bad cell ids: ", cellIds);
  outCellSet.GetIndices(1, cellIds);
  SVTKM_TEST_ASSERT((cellIds == svtkm::Id4(1, 2, 5, 4)), "Bad cell ids: ", cellIds);

  svtkm::cont::ArrayHandle<svtkm::Float32> outPointField;
  outData.GetField("pointvar").GetData().CopyTo(outPointField);
  SVTKM_TEST_ASSERT(outPointField.GetNumberOfValues() == 6,
                   "Wrong point field size: ",
                   outPointField.GetNumberOfValues());
  SVTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(1), 20.1),
                   "Bad point field value: ",
                   outPointField.GetPortalConstControl().Get(1));
  SVTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(4), 50.1),
                   "Bad point field value: ",
                   outPointField.GetPortalConstControl().Get(1));

  svtkm::cont::ArrayHandle<svtkm::Float32> outCellField;
  outData.GetField("cellvar").GetData().CopyTo(outCellField);
  SVTKM_TEST_ASSERT(outCellField.GetNumberOfValues() == 2, "Wrong cell field size.");
  SVTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(0), 100.1),
                   "Bad cell field value",
                   outCellField.GetPortalConstControl().Get(0));
  SVTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(1), 200.1),
                   "Bad cell field value",
                   outCellField.GetPortalConstControl().Get(0));
}

void TestPointMerging()
{
  svtkm::cont::testing::MakeTestDataSet makeDataSet;
  svtkm::cont::DataSet baseData = makeDataSet.Make3DUniformDataSet3(svtkm::Id3(4, 4, 4));

  svtkm::filter::Contour marchingCubes;
  marchingCubes.SetIsoValue(0.05);
  marchingCubes.SetMergeDuplicatePoints(false);
  marchingCubes.SetActiveField("pointvar");
  svtkm::cont::DataSet inData = marchingCubes.Execute(baseData);
  constexpr svtkm::Id originalNumPoints = 228;
  constexpr svtkm::Id originalNumCells = 76;
  SVTKM_TEST_ASSERT(inData.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  SVTKM_TEST_ASSERT(inData.GetNumberOfCells() == originalNumCells);

  svtkm::filter::CleanGrid cleanGrid;

  std::cout << "Clean grid without any merging" << std::endl;
  cleanGrid.SetCompactPointFields(false);
  cleanGrid.SetMergePoints(false);
  cleanGrid.SetRemoveDegenerateCells(false);
  svtkm::cont::DataSet noMerging = cleanGrid.Execute(inData);
  SVTKM_TEST_ASSERT(noMerging.GetNumberOfCells() == originalNumCells);
  SVTKM_TEST_ASSERT(noMerging.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  SVTKM_TEST_ASSERT(noMerging.GetNumberOfPoints() == originalNumPoints);
  SVTKM_TEST_ASSERT(noMerging.GetField("pointvar").GetNumberOfValues() == originalNumPoints);
  SVTKM_TEST_ASSERT(noMerging.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points" << std::endl;
  cleanGrid.SetMergePoints(true);
  cleanGrid.SetFastMerge(false);
  svtkm::cont::DataSet closeMerge = cleanGrid.Execute(inData);
  constexpr svtkm::Id closeMergeNumPoints = 62;
  SVTKM_TEST_ASSERT(closeMerge.GetNumberOfCells() == originalNumCells);
  SVTKM_TEST_ASSERT(closeMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeMerge.GetNumberOfPoints() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points with fast merge" << std::endl;
  cleanGrid.SetFastMerge(true);
  svtkm::cont::DataSet closeFastMerge = cleanGrid.Execute(inData);
  SVTKM_TEST_ASSERT(closeFastMerge.GetNumberOfCells() == originalNumCells);
  SVTKM_TEST_ASSERT(closeFastMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeFastMerge.GetNumberOfPoints() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeFastMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  SVTKM_TEST_ASSERT(closeFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points" << std::endl;
  cleanGrid.SetFastMerge(false);
  cleanGrid.SetTolerance(0.1);
  svtkm::cont::DataSet farMerge = cleanGrid.Execute(inData);
  constexpr svtkm::Id farMergeNumPoints = 36;
  SVTKM_TEST_ASSERT(farMerge.GetNumberOfCells() == originalNumCells);
  SVTKM_TEST_ASSERT(farMerge.GetCellSet().GetNumberOfPoints() == farMergeNumPoints);
  SVTKM_TEST_ASSERT(farMerge.GetNumberOfPoints() == farMergeNumPoints);
  SVTKM_TEST_ASSERT(farMerge.GetField("pointvar").GetNumberOfValues() == farMergeNumPoints);
  SVTKM_TEST_ASSERT(farMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly" << std::endl;
  cleanGrid.SetFastMerge(true);
  svtkm::cont::DataSet farFastMerge = cleanGrid.Execute(inData);
  constexpr svtkm::Id farFastMergeNumPoints = 19;
  SVTKM_TEST_ASSERT(farFastMerge.GetNumberOfCells() == originalNumCells);
  SVTKM_TEST_ASSERT(farFastMerge.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(farFastMerge.GetNumberOfPoints() == farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(farFastMerge.GetField("pointvar").GetNumberOfValues() == farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(farFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly with degenerate cells"
            << std::endl;
  cleanGrid.SetRemoveDegenerateCells(true);
  svtkm::cont::DataSet noDegenerateCells = cleanGrid.Execute(inData);
  constexpr svtkm::Id numNonDegenerateCells = 33;
  SVTKM_TEST_ASSERT(noDegenerateCells.GetNumberOfCells() == numNonDegenerateCells);
  SVTKM_TEST_ASSERT(noDegenerateCells.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(noDegenerateCells.GetNumberOfPoints() == farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(noDegenerateCells.GetField("pointvar").GetNumberOfValues() ==
                   farFastMergeNumPoints);
  SVTKM_TEST_ASSERT(noDegenerateCells.GetField("cellvar").GetNumberOfValues() ==
                   numNonDegenerateCells);
}

void RunTest()
{
  svtkm::filter::CleanGrid clean;

  std::cout << "*** Test with compact point fields on merge points off" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points off" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields on merge points on" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points on" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test point merging" << std::endl;
  TestPointMerging();
}

} // anonymous namespace

int UnitTestCleanGrid(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
