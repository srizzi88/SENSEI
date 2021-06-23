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
#include <svtkm/worklet/Tetrahedralize.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

using svtkm::cont::testing::MakeTestDataSet;

class TestingTetrahedralize
{
public:
  //
  // Create a uniform 3D structured cell set as input
  // Add a field which is the index type which is (i+j+k) % 2 to alternate tetrahedralization pattern
  // Points are all the same, but each hexahedron cell becomes 5 tetrahedral cells
  //
  void TestStructured() const
  {
    std::cout << "Testing TetrahedralizeStructured" << std::endl;
    using CellSetType = svtkm::cont::CellSetStructured<3>;
    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    // Create the input uniform cell set
    svtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    // Convert uniform hexahedra to tetrahedra
    svtkm::worklet::Tetrahedralize tetrahedralize;
    OutCellSetType outCellSet = tetrahedralize.Run(cellSet);

    // Create the output dataset with same coordinate system
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
  }

  //
  // Create an explicit 3D cell set as input and fill
  // Points are all the same, but each cell becomes tetrahedra
  //
  void TestExplicit() const
  {
    std::cout << "Testing TetrahedralizeExplicit" << std::endl;
    using CellSetType = svtkm::cont::CellSetExplicit<>;
    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    // Create the input explicit cell set
    svtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);
    svtkm::cont::ArrayHandle<svtkm::IdComponent> outCellsPerCell;

    // Convert explicit cells to tetrahedra
    svtkm::worklet::Tetrahedralize tetrahedralize;
    OutCellSetType outCellSet = tetrahedralize.Run(cellSet);

    // Create the output dataset explicit cell set with same coordinate system
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataSet.GetCoordinateSystem(0));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 11),
                     "Wrong result for Tetrahedralize filter");
  }

  void operator()() const
  {
    TestStructured();
    TestExplicit();
  }
};

int UnitTestTetrahedralize(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingTetrahedralize(), argc, argv);
}
