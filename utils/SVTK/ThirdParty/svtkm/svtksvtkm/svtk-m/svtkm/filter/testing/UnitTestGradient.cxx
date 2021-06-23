//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/Gradient.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestCellGradientUniform3D()
{
  std::cout << "Testing Gradient Filter with cell output on 3D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  svtkm::filter::Gradient gradient;
  gradient.SetOutputFieldName("Gradient");

  gradient.SetComputeVorticity(true);  //this won't work as we have a scalar field
  gradient.SetComputeQCriterion(true); //this won't work as we have a scalar field

  gradient.SetActiveField("pointvar");

  svtkm::cont::DataSet result = gradient.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasCellField("Gradient"), "Field missing.");

  //verify that the vorticity and qcriterion fields don't exist
  SVTKM_TEST_ASSERT(result.HasField("Vorticity") == false,
                   "scalar gradients can't generate vorticity");
  SVTKM_TEST_ASSERT(result.HasField("QCriterion") == false,
                   "scalar gradients can't generate qcriterion");

  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> resultArrayHandle;
  result.GetField("Gradient").GetData().CopyTo(resultArrayHandle);
  svtkm::Vec3f_64 expected[4] = {
    { 10.025, 30.075, 60.125 },
    { 10.025, 30.075, 60.125 },
    { 10.025, 30.075, 60.175 },
    { 10.025, 30.075, 60.175 },
  };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient filter on 3D uniform data");
  }
}

void TestCellGradientUniform3DWithVectorField()
{
  std::cout << "Testing Gradient Filter with vector cell output on 3D structured data" << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  svtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<svtkm::Vec3f_64> vec(nVerts);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = svtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> input = svtkm::cont::make_ArrayHandle(vec);
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec_pointvar", input);

  //we need to add Vec3 array to the dataset
  svtkm::filter::Gradient gradient;
  gradient.SetOutputFieldName("vec_gradient");
  gradient.SetComputeVorticity(true);
  gradient.SetComputeQCriterion(true);
  gradient.SetActiveField("vec_pointvar");

  svtkm::cont::DataSet result = gradient.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasCellField("vec_gradient"), "Result field missing.");

  //verify that the vorticity and qcriterion fields DO exist
  SVTKM_TEST_ASSERT(result.HasField("Vorticity") == true, "vec gradients should generate vorticity");
  SVTKM_TEST_ASSERT(result.HasField("QCriterion") == true,
                   "vec gradients should generate qcriterion");

  svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Vec3f_64, 3>> resultArrayHandle;
  result.GetCellField("vec_gradient").GetData().CopyTo(resultArrayHandle);
  svtkm::Vec<svtkm::Vec3f_64, 3> expected[4] = {
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    svtkm::Vec<svtkm::Vec3f_64, 3> e = expected[i];
    svtkm::Vec<svtkm::Vec3f_64, 3> r = resultArrayHandle.GetPortalConstControl().Get(i);

    SVTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
  }
}

void TestCellGradientExplicit()
{
  std::cout << "Testing Gradient Filter with cell output on Explicit data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  svtkm::filter::Gradient gradient;
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  svtkm::cont::DataSet result = gradient.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasCellField("gradient"), "Result field missing.");

  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> resultArrayHandle;
  result.GetCellField("gradient").GetData().CopyTo(resultArrayHandle);
  svtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, -0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient filter on 3D explicit data");
  }
}

void TestPointGradientUniform3DWithVectorField()
{
  std::cout << "Testing Gradient Filter with vector point output on 3D structured data"
            << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  svtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<svtkm::Vec3f_64> vec(nVerts);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = svtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> input = svtkm::cont::make_ArrayHandle(vec);
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec_pointvar", input);

  //we need to add Vec3 array to the dataset
  svtkm::filter::Gradient gradient;
  gradient.SetComputePointGradient(true);
  gradient.SetOutputFieldName("vec_gradient");
  gradient.SetActiveField("vec_pointvar");
  svtkm::cont::DataSet result = gradient.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasPointField("vec_gradient"), "Result field missing.");

  svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Vec3f_64, 3>> resultArrayHandle;
  result.GetPointField("vec_gradient").GetData().CopyTo(resultArrayHandle);
  svtkm::Vec<svtkm::Vec3f_64, 3> expected[4] = {
    { { 10.0, 10.0, 10.0 }, { 30.0, 30.0, 30.0 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.1, 60.1, 60.1 } },
    { { 10.0, 10.0, 10.0 }, { 30.1, 30.1, 30.1 }, { 60.2, 60.2, 60.2 } },
    { { 10.1, 10.1, 10.1 }, { 30.0, 30.0, 30.0 }, { 60.2, 60.2, 60.2 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    svtkm::Vec<svtkm::Vec3f_64, 3> e = expected[i];
    svtkm::Vec<svtkm::Vec3f_64, 3> r = resultArrayHandle.GetPortalConstControl().Get(i);

    SVTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field CellGradient filter on 3D uniform data");
  }
}

void TestPointGradientExplicit()
{
  std::cout << "Testing Gradient Filter with point output on Explicit data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  svtkm::filter::Gradient gradient;
  gradient.SetComputePointGradient(true);
  gradient.SetOutputFieldName("gradient");
  gradient.SetActiveField("pointvar");

  svtkm::cont::DataSet result = gradient.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasPointField("gradient"), "Result field missing.");

  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> resultArrayHandle;
  result.GetPointField("gradient").GetData().CopyTo(resultArrayHandle);

  svtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, 0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient filter on 3D explicit data");
  }
}

void TestGradient()
{
  TestCellGradientUniform3D();
  TestCellGradientUniform3DWithVectorField();
  TestCellGradientExplicit();

  TestPointGradientUniform3DWithVectorField();
  TestPointGradientExplicit();
}
}

int UnitTestGradient(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestGradient, argc, argv);
}
