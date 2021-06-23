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

#include <svtkm/filter/Mask.h>

using svtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingMask
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Setup and run filter to extract by stride
    svtkm::filter::Mask mask;
    svtkm::Id stride = 2;
    mask.SetStride(stride);

    svtkm::cont::DataSet output = mask.Execute(dataset);

    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8), "Wrong result for Mask");


    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                       cellFieldArray.GetPortalConstControl().Get(7) == 14.f,
                     "Wrong mask data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing mask cells uniform grid :" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Setup and run filter to extract by stride
    svtkm::filter::Mask mask;
    svtkm::Id stride = 9;
    mask.SetStride(stride);

    svtkm::cont::DataSet output = mask.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 7), "Wrong result for Mask");

    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 7 &&
                       cellFieldArray.GetPortalConstControl().Get(2) == 18.f,
                     "Wrong mask data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing mask cells explicit:" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Setup and run filter to extract by stride
    svtkm::filter::Mask mask;
    svtkm::Id stride = 2;
    mask.SetStride(stride);

    svtkm::cont::DataSet output = mask.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 2), "Wrong result for Mask");

    svtkm::cont::ArrayHandle<svtkm::Float32> cellFieldArray;
    output.GetField("cellvar").GetData().CopyTo(cellFieldArray);

    SVTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                       cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong mask data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit();
  }
};
}

int UnitTestMaskFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingMask(), argc, argv);
}
