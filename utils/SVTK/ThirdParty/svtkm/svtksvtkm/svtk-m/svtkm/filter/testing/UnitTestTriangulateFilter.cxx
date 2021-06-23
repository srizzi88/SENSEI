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

#include <svtkm/filter/Triangulate.h>

using svtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingTriangulate
{
public:
  void TestStructured() const
  {
    std::cout << "Testing triangulate structured" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    svtkm::filter::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = triangulate.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 32), "Wrong result for Triangulate");
    SVTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 25),
                     "Wrong number of points for Triangulate");

    svtkm::cont::ArrayHandle<svtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();

    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(2) == 1.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(3) == 1.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(30) == 15.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(31) == 15.f, "Wrong cell field data");
  }

  void TestExplicit() const
  {
    std::cout << "Testing triangulate explicit" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DExplicitDataSet0();
    svtkm::filter::Triangulate triangulate;
    triangulate.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = triangulate.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 14), "Wrong result for Triangulate");
    SVTKM_TEST_ASSERT(test_equal(output.GetField("pointvar").GetNumberOfValues(), 16),
                     "Wrong number of points for Triangulate");

    svtkm::cont::ArrayHandle<svtkm::Float32> outData =
      output.GetField("cellvar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Float32>>();

    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(1) == 1.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(2) == 1.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(5) == 3.f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outData.GetPortalConstControl().Get(6) == 3.f, "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestStructured();
    this->TestExplicit();
  }
};
}

int UnitTestTriangulateFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingTriangulate(), argc, argv);
}
