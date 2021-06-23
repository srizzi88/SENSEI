//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/ExtractGeometry.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

using svtkm::cont::testing::MakeTestDataSet;

class TestingExtractGeometry
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitById() const
  {
    std::cout << "Testing extract cell explicit by id:" << std::endl;

    using CellSetType = svtkm::cont::CellSetExplicit<>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 2;
    svtkm::Id cellids[nCells] = { 1, 2 };
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds = svtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set with cell set containing extracted cells and all points
    svtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 110.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box) on explicit:" << std::endl;

    using CellSetType = svtkm::cont::CellSetExplicit<>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    svtkm::Vec3f minPoint(0.5f, 0.0f, 0.0f);
    svtkm::Vec3f maxPoint(2.0f, 2.0f, 2.0f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted cells and all points
    svtkm::worklet::ExtractGeometry extractGeometry;
    svtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coordinates"),
                          svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);


    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById2D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<2>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;


    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 5;
    svtkm::Id cellids[nCells] = { 0, 4, 5, 10, 15 };
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds = svtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set permutation of with only extracted cells
    svtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 4.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById3D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<3>;
    using OutCellSetType = svtkm::cont::CellSetPermutation<CellSetType>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Cells to extract
    const int nCells = 5;
    svtkm::Id cellids[nCells] = { 0, 4, 5, 10, 15 };
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds = svtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set with cell set containing extracted cells and all points
    svtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = extractGeometry.Run(cellSet, cellIds);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells),
                     "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 5.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box):" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<3>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    svtkm::Vec3f minPoint(1.0f, 1.0f, 1.0f);
    svtkm::Vec3f maxPoint(3.0f, 3.0f, 3.0f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted points
    svtkm::worklet::ExtractGeometry extractGeometry;
    svtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coords"),
                          svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(0) == 21.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformBySphere() const
  {
    std::cout << "Testing extract cells with implicit function (sphere):" << std::endl;

    using CellSetType = svtkm::cont::CellSetStructured<3>;

    // Input data set created
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet().CopyTo(cellSet);

    // Implicit function
    svtkm::Vec3f center(2.f, 2.f, 2.f);
    svtkm::FloatDefault radius(1.8f);

    bool extractInside = true;
    bool extractBoundaryCells = false;
    bool extractOnlyBoundaryCells = false;

    // Output data set with cell set containing extracted cells
    svtkm::worklet::ExtractGeometry extractGeometry;
    svtkm::cont::DynamicCellSet outCellSet =
      extractGeometry.Run(cellSet,
                          dataset.GetCoordinateSystem("coords"),
                          svtkm::cont::make_ImplicitFunctionHandle<svtkm::Sphere>(center, radius),
                          extractInside,
                          extractBoundaryCells,
                          extractOnlyBoundaryCells);

    auto cellvar =
      dataset.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();
    auto cellFieldArray = extractGeometry.ProcessCellField(cellvar);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 22.f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniformById2D();
    this->TestUniformById3D();
    this->TestUniformBySphere();
    this->TestUniformByBox();
    this->TestExplicitById();
    this->TestExplicitByBox();
  }
};

int UnitTestExtractGeometry(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingExtractGeometry(), argc, argv);
}
