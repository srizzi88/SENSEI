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
#include <svtkm/filter/CellAverage.h>

namespace
{

void TestCellAverageRegular3D()
{
  std::cout << "Testing CellAverage Filter on 3D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  svtkm::filter::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("avgvals");
  cellAverage.SetActiveField("pointvar");
  svtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasCellField("avgvals") == true, "Result field not present.");

  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  result.GetCellField("avgvals").GetData().CopyTo(resultArrayHandle);
  {
    svtkm::Float32 expected[4] = { 60.1875f, 70.2125f, 120.3375f, 130.3625f };
    for (svtkm::Id i = 0; i < 4; ++i)
    {
      SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                       "Wrong result for CellAverage worklet on 3D regular data");
    }
  }

  std::cout << "Run again for point coordinates" << std::endl;
  cellAverage.SetOutputFieldName("avgpos");
  cellAverage.SetUseCoordinateSystemAsField(true);
  result = cellAverage.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasCellField("avgpos"), "Result field not present.");

  svtkm::cont::ArrayHandle<svtkm::Vec3f> resultPointArray;
  svtkm::cont::Field resultPointField = result.GetCellField("avgpos");
  resultPointField.GetData().CopyTo(resultPointArray);
  {
    svtkm::FloatDefault expected[4][3] = {
      { 0.5f, 0.5f, 0.5f }, { 1.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 1.5f }, { 1.5f, 0.5f, 1.5f }
    };
    for (svtkm::Id i = 0; i < 4; ++i)
    {
      svtkm::Vec3f expectedVec(expected[i][0], expected[i][1], expected[i][2]);
      svtkm::Vec3f computedVec(resultPointArray.GetPortalConstControl().Get(i));
      SVTKM_TEST_ASSERT(test_equal(computedVec, expectedVec),
                       "Wrong result for CellAverage worklet on 3D regular data");
    }
  }
}

void TestCellAverageRegular2D()
{
  std::cout << "Testing CellAverage Filter on 2D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  svtkm::filter::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  svtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  SVTKM_TEST_ASSERT(result.HasPointField("pointvar"), "Field missing.");

  svtkm::cont::Field resultField = result.GetCellField("pointvar");
  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  resultField.GetData().CopyTo(resultArrayHandle);
  svtkm::Float32 expected[2] = { 30.1f, 40.1f };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2D regular data");
  }
}

void TestCellAverageExplicit()
{
  std::cout << "Testing CellAverage Filter on Explicit data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();

  svtkm::filter::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar");

  svtkm::cont::DataSet result = cellAverage.Execute(dataSet);

  // If no name is given, should have the same name as the input.
  SVTKM_TEST_ASSERT(result.HasCellField("pointvar"), "Field missing.");

  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  svtkm::cont::Field resultField = result.GetCellField("pointvar");
  resultField.GetData().CopyTo(resultArrayHandle);
  svtkm::Float32 expected[2] = { 20.1333f, 35.2f };
  for (int i = 0; i < 2; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 3D regular data");
  }
}

void TestCellAverage()
{
  TestCellAverageRegular2D();
  TestCellAverageRegular3D();
  TestCellAverageExplicit();
}
}

int UnitTestCellAverageFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellAverage, argc, argv);
}
