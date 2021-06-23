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
#include <svtkm/worklet/WarpScalar.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetFieldAdd.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{
template <typename T>
svtkm::cont::DataSet MakeWarpScalarTestDataSet()
{
  svtkm::cont::DataSet dataSet;

  std::vector<svtkm::Vec<T, 3>> coordinates;
  std::vector<T> scaleFactor;
  const svtkm::Id dim = 5;
  for (svtkm::Id i = 0; i < dim; ++i)
  {
    T z = static_cast<T>(i);
    for (svtkm::Id j = 0; j < dim; ++j)
    {
      T x = static_cast<T>(j);
      T y = static_cast<T>(j + 1);
      coordinates.push_back(svtkm::make_Vec(x, y, z));
      scaleFactor.push_back(static_cast<T>(i * dim + j));
    }
  }

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, svtkm::CopyFlag::On));
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "scalefactor", scaleFactor);
  return dataSet;
}
}

void TestWarpScalar()
{
  std::cout << "Testing WarpScalar Worklet" << std::endl;
  using vecType = svtkm::Vec3f;

  svtkm::cont::DataSet ds = MakeWarpScalarTestDataSet<svtkm::FloatDefault>();

  svtkm::FloatDefault scaleAmount = 2;
  svtkm::cont::ArrayHandle<vecType> result;

  vecType normal = svtkm::make_Vec<svtkm::FloatDefault>(static_cast<svtkm::FloatDefault>(0.0),
                                                      static_cast<svtkm::FloatDefault>(0.0),
                                                      static_cast<svtkm::FloatDefault>(1.0));
  auto coordinate = ds.GetCoordinateSystem().GetData();
  svtkm::Id nov = coordinate.GetNumberOfValues();
  svtkm::cont::ArrayHandleConstant<vecType> normalAH =
    svtkm::cont::make_ArrayHandleConstant(normal, nov);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> scaleFactorArray;
  auto scaleFactor = ds.GetField("scalefactor").GetData().ResetTypes(svtkm::TypeListFieldScalar());
  scaleFactor.CopyTo(scaleFactorArray);
  auto sFAPortal = scaleFactorArray.GetPortalControl();

  svtkm::worklet::WarpScalar warpWorklet;
  warpWorklet.Run(ds.GetCoordinateSystem(), normalAH, scaleFactor, scaleAmount, result);
  auto resultPortal = result.GetPortalConstControl();

  for (svtkm::Id i = 0; i < nov; i++)
  {
    for (svtkm::Id j = 0; j < 3; j++)
    {
      svtkm::FloatDefault ans =
        coordinate.GetPortalConstControl().Get(i)[static_cast<svtkm::IdComponent>(j)] +
        scaleAmount * normal[static_cast<svtkm::IdComponent>(j)] * sFAPortal.Get(i);
      SVTKM_TEST_ASSERT(test_equal(ans, resultPortal.Get(i)[static_cast<svtkm::IdComponent>(j)]),
                       " Wrong result for WarpVector worklet");
    }
  }
}

int UnitTestWarpScalar(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWarpScalar, argc, argv);
}
