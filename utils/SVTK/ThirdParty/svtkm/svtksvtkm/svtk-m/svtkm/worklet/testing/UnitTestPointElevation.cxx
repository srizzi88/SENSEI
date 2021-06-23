//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/PointElevation.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>

#include <svtkm/cont/testing/Testing.h>

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

void TestPointElevation()
{
  std::cout << "Testing PointElevation Worklet" << std::endl;

  svtkm::cont::DataSet dataSet = MakePointElevationTestDataSet();

  svtkm::cont::ArrayHandle<svtkm::Float32> result;

  svtkm::worklet::PointElevation pointElevationWorklet;
  pointElevationWorklet.SetLowPoint(svtkm::make_Vec<svtkm::Float64>(0.0, 0.0, 0.0));
  pointElevationWorklet.SetHighPoint(svtkm::make_Vec<svtkm::Float64>(0.0, 1.0, 0.0));
  pointElevationWorklet.SetRange(0.0, 2.0);

  svtkm::worklet::DispatcherMapField<svtkm::worklet::PointElevation> dispatcher(
    pointElevationWorklet);
  dispatcher.Invoke(dataSet.GetCoordinateSystem(), result);

  auto coordinates = dataSet.GetCoordinateSystem().GetData();
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                result.GetPortalConstControl().Get(i)),
                     "Wrong result for PointElevation worklet");
  }
}

int UnitTestPointElevation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestPointElevation, argc, argv);
}
