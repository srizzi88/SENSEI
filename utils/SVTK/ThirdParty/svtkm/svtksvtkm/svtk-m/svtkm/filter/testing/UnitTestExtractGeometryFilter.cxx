//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/ExtractGeometry.h>

using svtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingExtractGeometry
{
public:
  void TestUniformByBox0() const
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    svtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    svtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    auto box = svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    svtkm::filter::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(false);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    svtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for ExtractGeometry");

    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 21.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(7) == 42.f, "Wrong cell field data");
  }

  void TestUniformByBox1() const
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    svtkm::Vec3f minPoint(1.f, 1.f, 1.f);
    svtkm::Vec3f maxPoint(3.f, 3.f, 3.f);
    auto box = svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    svtkm::filter::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(false);
    extractGeometry.SetExtractBoundaryCells(false);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    svtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 56), "Wrong result for ExtractGeometry");

    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(55) == 63.f, "Wrong cell field data");
  }

  void TestUniformByBox2() const
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    svtkm::Vec3f minPoint(0.5f, 0.5f, 0.5f);
    svtkm::Vec3f maxPoint(3.5f, 3.5f, 3.5f);
    auto box = svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    svtkm::filter::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(true);
    extractGeometry.SetExtractOnlyBoundaryCells(false);

    svtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 64), "Wrong result for ExtractGeometry");

    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(63) == 63.f, "Wrong cell field data");
  }
  void TestUniformByBox3() const
  {
    std::cout << "Testing extract geometry with implicit function (box):" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Implicit function
    svtkm::Vec3f minPoint(0.5f, 0.5f, 0.5f);
    svtkm::Vec3f maxPoint(3.5f, 3.5f, 3.5f);
    auto box = svtkm::cont::make_ImplicitFunctionHandle<svtkm::Box>(minPoint, maxPoint);

    // Setup and run filter to extract by volume of interest
    svtkm::filter::ExtractGeometry extractGeometry;
    extractGeometry.SetImplicitFunction(box);
    extractGeometry.SetExtractInside(true);
    extractGeometry.SetExtractBoundaryCells(true);
    extractGeometry.SetExtractOnlyBoundaryCells(true);

    svtkm::cont::DataSet output = extractGeometry.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 56), "Wrong result for ExtractGeometry");

    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(55) == 63.f, "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniformByBox0();
    this->TestUniformByBox1();
    this->TestUniformByBox2();
    this->TestUniformByBox3();
  }
};
}

int UnitTestExtractGeometryFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingExtractGeometry(), argc, argv);
}
