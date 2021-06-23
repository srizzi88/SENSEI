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
#include <svtkm/worklet/WarpVector.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{
template <typename T>
svtkm::cont::DataSet MakeWarpVectorTestDataSet()
{
  svtkm::cont::DataSet dataSet;

  std::vector<svtkm::Vec<T, 3>> coordinates;
  const svtkm::Id dim = 5;
  for (svtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / 2.0f;
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

void TestWarpVector()
{
  std::cout << "Testing WarpVector Worklet" << std::endl;
  using vecType = svtkm::Vec3f;

  svtkm::cont::DataSet ds = MakeWarpVectorTestDataSet<svtkm::FloatDefault>();
  svtkm::cont::ArrayHandle<vecType> result;

  svtkm::FloatDefault scale = 2;

  vecType vector = svtkm::make_Vec<svtkm::FloatDefault>(static_cast<svtkm::FloatDefault>(0.0),
                                                      static_cast<svtkm::FloatDefault>(0.0),
                                                      static_cast<svtkm::FloatDefault>(2.0));
  auto coordinate = ds.GetCoordinateSystem().GetData();
  svtkm::Id nov = coordinate.GetNumberOfValues();
  svtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    svtkm::cont::make_ArrayHandleConstant(vector, nov);

  svtkm::worklet::WarpVector warpWorklet;
  warpWorklet.Run(ds.GetCoordinateSystem(), vectorAH, scale, result);
  auto resultPortal = result.GetPortalConstControl();
  for (svtkm::Id i = 0; i < nov; i++)
  {
    for (svtkm::Id j = 0; j < 3; j++)
    {
      svtkm::FloatDefault ans =
        coordinate.GetPortalConstControl().Get(i)[static_cast<svtkm::IdComponent>(j)] +
        scale * vector[static_cast<svtkm::IdComponent>(j)];
      SVTKM_TEST_ASSERT(test_equal(ans, resultPortal.Get(i)[static_cast<svtkm::IdComponent>(j)]),
                       " Wrong result for WarpVector worklet");
    }
  }
}

int UnitTestWarpVector(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWarpVector, argc, argv);
}
