//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/Gradient.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestCellGradientUniform2D()
{
  std::cout << "Testing CellGradient Worklet on 2D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  svtkm::cont::ArrayHandle<svtkm::Float32> input;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> result;

  dataSet.GetField("pointvar").GetData().CopyTo(input);

  svtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  svtkm::Vec3f_32 expected[2] = { { 10, 30, 0 }, { 10, 30, 0 } };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 2D uniform data");
  }
}

void TestCellGradientUniform3D()
{
  std::cout << "Testing CellGradient Worklet on 3D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  svtkm::cont::ArrayHandle<svtkm::Float32> input;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> result;

  dataSet.GetField("pointvar").GetData().CopyTo(input);

  svtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  svtkm::Vec3f_32 expected[4] = {
    { 10.025f, 30.075f, 60.125f },
    { 10.025f, 30.075f, 60.125f },
    { 10.025f, 30.075f, 60.175f },
    { 10.025f, 30.075f, 60.175f },
  };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 3D uniform data");
  }
}

void TestCellGradientUniform3DWithVectorField()
{
  std::cout
    << "Testing CellGradient and QCriterion Worklet with a vector field on 3D structured data"
    << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  svtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<svtkm::Vec3f_64> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = svtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> input = svtkm::cont::make_ArrayHandle(vec);

  //we need to add Vec3 array to the dataset
  svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Vec3f_64, 3>> result;

  svtkm::worklet::GradientOutputFields<svtkm::Vec3f_64> extraOutput;
  extraOutput.SetComputeDivergence(false);
  extraOutput.SetComputeVorticity(false);
  extraOutput.SetComputeQCriterion(true);

  svtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, extraOutput);

  SVTKM_TEST_ASSERT((extraOutput.Gradient.GetNumberOfValues() == 4),
                   "Gradient field should be generated");
  SVTKM_TEST_ASSERT((extraOutput.Divergence.GetNumberOfValues() == 0),
                   "Divergence field shouldn't be generated");
  SVTKM_TEST_ASSERT((extraOutput.Vorticity.GetNumberOfValues() == 0),
                   "Vorticity field shouldn't be generated");
  SVTKM_TEST_ASSERT((extraOutput.QCriterion.GetNumberOfValues() == 4),
                   "QCriterion field should be generated");

  svtkm::Vec<svtkm::Vec3f_64, 3> expected[4] = {
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } }
  };
  for (int i = 0; i < 4; ++i)
  {
    svtkm::Vec<svtkm::Vec3f_64, 3> e = expected[i];
    svtkm::Vec<svtkm::Vec3f_64, 3> r = result.GetPortalConstControl().Get(i);

    SVTKM_TEST_ASSERT(test_equal(e[0], r[0]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[1], r[1]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");
    SVTKM_TEST_ASSERT(test_equal(e[2], r[2]),
                     "Wrong result for vec field CellGradient worklet on 3D uniform data");

    const svtkm::Vec3f_64 v(e[1][2] - e[2][1], e[2][0] - e[0][2], e[0][1] - e[1][0]);
    const svtkm::Vec3f_64 s(e[1][2] + e[2][1], e[2][0] + e[0][2], e[0][1] + e[1][0]);
    const svtkm::Vec3f_64 d(e[0][0], e[1][1], e[2][2]);

    //compute QCriterion
    svtkm::Float64 qcriterion =
      ((svtkm::Dot(v, v) / 2.0f) - (svtkm::Dot(d, d) + (svtkm::Dot(s, s) / 2.0f))) / 2.0f;

    svtkm::Float64 q = extraOutput.QCriterion.GetPortalConstControl().Get(i);

    std::cout << "qcriterion expected: " << qcriterion << std::endl;
    std::cout << "qcriterion actual: " << q << std::endl;

    SVTKM_TEST_ASSERT(
      test_equal(qcriterion, q),
      "Wrong result for QCriterion field of CellGradient worklet on 3D uniform data");
  }
}

void TestCellGradientUniform3DWithVectorField2()
{
  std::cout << "Testing CellGradient Worklet with a vector field on 3D structured data" << std::endl
            << "Disabling Gradient computation and enabling Divergence, and Vorticity" << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  //Verify that we can compute the gradient of a 3 component vector
  const int nVerts = 18;
  svtkm::Float64 vars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };
  std::vector<svtkm::Vec3f_64> vec(18);
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    vec[i] = svtkm::make_Vec(vars[i], vars[i], vars[i]);
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> input = svtkm::cont::make_ArrayHandle(vec);

  svtkm::worklet::GradientOutputFields<svtkm::Vec3f_64> extraOutput;
  extraOutput.SetComputeGradient(false);
  extraOutput.SetComputeDivergence(true);
  extraOutput.SetComputeVorticity(true);
  extraOutput.SetComputeQCriterion(false);

  svtkm::worklet::CellGradient gradient;
  auto result =
    gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input, extraOutput);

  //Verify that the result is 0 size
  SVTKM_TEST_ASSERT((result.GetNumberOfValues() == 0), "Gradient field shouldn't be generated");
  //Verify that the extra arrays are the correct size
  SVTKM_TEST_ASSERT((extraOutput.Gradient.GetNumberOfValues() == 0),
                   "Gradient field shouldn't be generated");
  SVTKM_TEST_ASSERT((extraOutput.Divergence.GetNumberOfValues() == 4),
                   "Divergence field should be generated");
  SVTKM_TEST_ASSERT((extraOutput.Vorticity.GetNumberOfValues() == 4),
                   "Vorticity field should be generated");
  SVTKM_TEST_ASSERT((extraOutput.QCriterion.GetNumberOfValues() == 0),
                   "QCriterion field shouldn't be generated");

  //Verify the contents of the other arrays
  svtkm::Vec<svtkm::Vec3f_64, 3> expected_gradients[4] = {
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.125, 60.125, 60.125 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } },
    { { 10.025, 10.025, 10.025 }, { 30.075, 30.075, 30.075 }, { 60.175, 60.175, 60.175 } }
  };

  for (int i = 0; i < 4; ++i)
  {
    svtkm::Vec<svtkm::Vec3f_64, 3> eg = expected_gradients[i];

    svtkm::Float64 d = extraOutput.Divergence.GetPortalConstControl().Get(i);
    SVTKM_TEST_ASSERT(test_equal((eg[0][0] + eg[1][1] + eg[2][2]), d),
                     "Wrong result for Divergence on 3D uniform data");

    svtkm::Vec3f_64 ev(eg[1][2] - eg[2][1], eg[2][0] - eg[0][2], eg[0][1] - eg[1][0]);
    svtkm::Vec3f_64 v = extraOutput.Vorticity.GetPortalConstControl().Get(i);
    SVTKM_TEST_ASSERT(test_equal(ev, v), "Wrong result for Vorticity on 3D uniform data");
  }
}

void TestCellGradientExplicit()
{
  std::cout << "Testing CellGradient Worklet on Explicit data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  svtkm::cont::ArrayHandle<svtkm::Float32> input;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> result;
  dataSet.GetField("pointvar").GetData().CopyTo(input);

  svtkm::worklet::CellGradient gradient;
  result = gradient.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), input);

  svtkm::Vec3f_32 expected[2] = { { 10.f, 10.1f, 0.0f }, { 10.f, 10.1f, -0.0f } };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellGradient worklet on 3D explicit data");
  }
}

void TestCellGradient()
{
  TestCellGradientUniform2D();
  TestCellGradientUniform3D();
  TestCellGradientUniform3DWithVectorField();
  TestCellGradientUniform3DWithVectorField2();
  TestCellGradientExplicit();
}
}

int UnitTestCellGradient(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellGradient, argc, argv);
}
