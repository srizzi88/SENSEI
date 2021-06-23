//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/Threshold.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ArrayPortalToIterators.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{

class HasValue
{
public:
  SVTKM_CONT
  HasValue(svtkm::Float32 value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(ScalarType value) const
  {
    return static_cast<svtkm::Float32>(value) == this->Value;
  }

private:
  svtkm::Float32 Value;
};

using svtkm::cont::testing::MakeTestDataSet;

class TestingThreshold
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing threshold on 2D uniform dataset" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<2>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    svtkm::cont::ArrayHandle<svtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().CopyTo(pointvar);

    svtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, pointvar, svtkm::cont::Field::Association::POINTS, HasValue(60.1f));

    SVTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 200.1f,
                     "Wrong cell field data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing threshold on 3D uniform dataset" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<3>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    svtkm::cont::ArrayHandle<svtkm::Float32> pointvar;
    dataset.GetField("pointvar").GetData().CopyTo(pointvar);

    svtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, pointvar, svtkm::cont::Field::Association::POINTS, HasValue(20.1f));

    SVTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells");

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);
    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 100.2f,
                     "Wrong cell field data");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    using CellSetType = svtkm::cont::CellSetExplicit<>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet0();

    CellSetType cellset;
    dataset.GetCellSet().CopyTo(cellset);

    svtkm::cont::ArrayHandle<svtkm::Float32> cellvar;
    dataset.GetField("cellvar").GetData().CopyTo(cellvar);

    svtkm::worklet::Threshold threshold;
    OutCellSetType outCellSet =
      threshold.Run(cellset, cellvar, svtkm::cont::Field::Association::CELL_SET, HasValue(100.1f));

    SVTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 1, "Wrong number of cells");

    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray = threshold.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 1 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 100.1f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};
}

int UnitTestThreshold(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingThreshold(), argc, argv);
}
