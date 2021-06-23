//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/Triangulate.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

using svtkm::cont::testing::MakeTestDataSet;

class TestingTriangulate
{
public:
  void TestStructured() const
  {
    std::cout << "Testing TriangulateStructured:" << std::endl;
    using CellSetType = svtkm::cont::CellSetStructured<2>;
    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    svtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    // Convert uniform quadrilaterals to triangles
    svtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet);

    // Create the output dataset and assign the input coordinate system
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    // Two triangles are created for every quad cell
    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 2),
                     "Wrong result for Triangulate filter");
  }

  void TestExplicit() const
  {
    std::cout << "Testing TriangulateExplicit:" << std::endl;
    using CellSetType = svtkm::cont::CellSetExplicit<>;
    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    svtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DExplicitDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);
    svtkm::cont::ArrayHandle<svtkm::IdComponent> outCellsPerCell;

    // Convert explicit cells to triangles
    svtkm::worklet::Triangulate triangulate;
    OutCellSetType outCellSet = triangulate.Run(cellSet);

    // Create the output dataset explicit cell set with same coordinate system
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 14),
                     "Wrong result for Triangulate filter");
  }

  void operator()() const
  {
    TestStructured();
    TestExplicit();
  }
};

int UnitTestTriangulate(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingTriangulate(), argc, argv);
}
