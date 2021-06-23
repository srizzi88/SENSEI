//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/PointElevation.h>

#include <vector>

namespace
{

svtkm::cont::DataSet MakePointElevationTestDataSet()
{
  svtkm::cont::DataSet dataSet;

  std::vector<svtkm::Vec3f_32> coordinates;
  const svtkm::Id dim = 5;
  for (svtkm::Id j = 0; j < dim; ++j)
  {
    svtkm::Float32 z = static_cast<svtkm::Float32>(j) / static_cast<svtkm::Float32>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      svtkm::Float32 x = static_cast<svtkm::Float32>(i) / static_cast<svtkm::Float32>(dim - 1);
      svtkm::Float32 y = (x * x + z * z) / 2.0f;
      coordinates.push_back(svtkm::make_Vec(x, y, z));
    }
  }

  svtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, svtkm::CopyFlag::On));

  svtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (svtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (svtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(svtkm::CELL_SHAPE_QUAD,
                      4,
                      svtkm::make_Vec<svtkm::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(svtkm::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}
}

void TestPointElevationNoPolicy()
{
  std::cout << "Testing PointElevation Filter With No Policy" << std::endl;

  svtkm::cont::DataSet inputData = MakePointElevationTestDataSet();

  svtkm::filter::PointElevation filter;
  filter.SetLowPoint(0.0, 0.0, 0.0);
  filter.SetHighPoint(0.0, 1.0, 0.0);
  filter.SetRange(0.0, 2.0);

  filter.SetOutputFieldName("height");
  filter.SetUseCoordinateSystemAsField(true);
  auto result = filter.Execute(inputData);

  //verify the result
  SVTKM_TEST_ASSERT(result.HasPointField("height"), "Output field missing.");

  svtkm::cont::ArrayHandle<svtkm::Float64> resultArrayHandle;
  result.GetPointField("height").GetData().CopyTo(resultArrayHandle);
  auto coordinates = inputData.GetCoordinateSystem().GetData();
  for (svtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                resultArrayHandle.GetPortalConstControl().Get(i)),
                     "Wrong result for PointElevation worklet");
  }
}

void TestPointElevationWithPolicy()
{

  //simple test
  std::cout << "Testing PointElevation Filter With Explicit Policy" << std::endl;

  svtkm::cont::DataSet inputData = MakePointElevationTestDataSet();

  svtkm::filter::PointElevation filter;
  filter.SetLowPoint(0.0, 0.0, 0.0);
  filter.SetHighPoint(0.0, 1.0, 0.0);
  filter.SetRange(0.0, 2.0);
  filter.SetUseCoordinateSystemAsField(true);

  svtkm::filter::PolicyDefault p;
  auto result = filter.Execute(inputData, p);

  //verify the result
  SVTKM_TEST_ASSERT(result.HasPointField("elevation"), "Output field has wrong association");

  svtkm::cont::ArrayHandle<svtkm::Float64> resultArrayHandle;
  result.GetPointField("elevation").GetData().CopyTo(resultArrayHandle);
  auto coordinates = inputData.GetCoordinateSystem().GetData();
  for (svtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                resultArrayHandle.GetPortalConstControl().Get(i)),
                     "Wrong result for PointElevation worklet");
  }
}

void TestPointElevation()
{
  TestPointElevationNoPolicy();
  TestPointElevationWithPolicy();
}

int UnitTestPointElevationFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestPointElevation, argc, argv);
}
