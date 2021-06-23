//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/MaskPoints.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{

using svtkm::cont::testing::MakeTestDataSet;

class TestingMaskPoints
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing mask points stride on 2D uniform dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Output dataset contains input coordinate system
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output dataset gets new cell set of points that pass subsampling
    svtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(dataset.GetCellSet(), 2);
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 12), "Wrong result for MaskPoints");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing mask points stride on 3D uniform dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Output dataset contains input coordinate system and point data
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output dataset gets new cell set of points that meet threshold predicate
    svtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(dataset.GetCellSet(), 5);
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 25), "Wrong result for MaskPoints");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing mask points stride on 3D explicit dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Output dataset contains input coordinate system and point data
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output dataset gets new cell set of points that meet threshold predicate
    svtkm::worklet::MaskPoints maskPoints;
    OutCellSetType outCellSet;
    outCellSet = maskPoints.Run(dataset.GetCellSet(), 3);
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 3), "Wrong result for MaskPoints");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};
}

int UnitTestMaskPoints(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingMaskPoints(), argc, argv);
}
