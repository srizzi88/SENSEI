//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/Mask.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

using svtkm::cont::testing::MakeTestDataSet;

class TestingMask
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniform2D() const
  {
    std::cout << "Testing mask cells structured:" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<2>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set permutation
    svtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 2);

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for Mask");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(7) == 14.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniform3D() const
  {
    std::cout << "Testing mask cells structured:" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<3>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;
    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set with cell set permuted
    svtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 9);

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 7), "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 18.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicit() const
  {
    std::cout << "Testing mask cells explicit:" << std::endl;

    using CellSetType = svtkm::cont::CellSetExplicit<>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Output data set with cell set permuted
    svtkm::worklet::Mask maskCells;
    OutCellSetType outCellSet = maskCells.Run(cellSet, 2);

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = maskCells.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit();
  }
};

int UnitTestMask(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingMask(), argc, argv);
}
