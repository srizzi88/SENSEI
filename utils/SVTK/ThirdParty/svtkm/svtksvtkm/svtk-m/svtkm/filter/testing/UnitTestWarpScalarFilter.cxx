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
#include <svtkm/filter/WarpScalar.h>

#include <random>
#include <vector>

namespace
{
const svtkm::Id dim = 5;
template <typename T>
svtkm::cont::DataSet MakeWarpScalarTestDataSet()
{
  using vecType = svtkm::Vec<T, 3>;
  svtkm::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  std::vector<T> scalarFactor;
  for (svtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(svtkm::make_Vec(x, y, z));
      vec1.push_back(svtkm::make_Vec(x, y, y));
      scalarFactor.push_back(static_cast<T>(j * dim + i));
    }
  }

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, svtkm::CopyFlag::On));

  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec1", vec1);
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "scalarfactor", scalarFactor);

  vecType normal = svtkm::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
  svtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    svtkm::cont::make_ArrayHandleConstant(normal, dim * dim);
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "normal", vectorAH);

  return dataSet;
}

void CheckResult(const svtkm::filter::WarpScalar& filter, const svtkm::cont::DataSet& result)
{
  SVTKM_TEST_ASSERT(result.HasPointField("warpscalar"), "Output filed warpscalar is missing");
  using vecType = svtkm::Vec3f;
  svtkm::cont::ArrayHandle<vecType> outputArray;
  result.GetPointField("warpscalar").GetData().CopyTo(outputArray);
  auto outPortal = outputArray.GetPortalConstControl();

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> sfArray;
  result.GetPointField("scalarfactor").GetData().CopyTo(sfArray);
  auto sfPortal = sfArray.GetPortalConstControl();

  for (svtkm::Id j = 0; j < dim; ++j)
  {
    svtkm::FloatDefault z =
      static_cast<svtkm::FloatDefault>(j) / static_cast<svtkm::FloatDefault>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      svtkm::FloatDefault x =
        static_cast<svtkm::FloatDefault>(i) / static_cast<svtkm::FloatDefault>(dim - 1);
      svtkm::FloatDefault y = (x * x + z * z) / static_cast<svtkm::FloatDefault>(2.0);
      svtkm::FloatDefault targetZ = filter.GetUseCoordinateSystemAsField()
        ? z + static_cast<svtkm::FloatDefault>(2 * sfPortal.Get(j * dim + i))
        : y + static_cast<svtkm::FloatDefault>(2 * sfPortal.Get(j * dim + i));
      auto point = outPortal.Get(j * dim + i);
      SVTKM_TEST_ASSERT(point[0] == x, "Wrong result of x value for warp scalar");
      SVTKM_TEST_ASSERT(point[1] == y, "Wrong result of y value for warp scalar");
      SVTKM_TEST_ASSERT(point[2] == targetZ, "Wrong result of z value for warp scalar");
    }
  }
}

void TestWarpScalarFilter()
{
  std::cout << "Testing WarpScalar filter" << std::endl;
  svtkm::cont::DataSet ds = MakeWarpScalarTestDataSet<svtkm::FloatDefault>();
  svtkm::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    svtkm::filter::WarpScalar filter(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetNormalField("normal");
    filter.SetScalarFactorField("scalarfactor");
    svtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    svtkm::filter::WarpScalar filter(scale);
    filter.SetActiveField("vec1");
    filter.SetNormalField("normal");
    filter.SetScalarFactorField("scalarfactor");
    svtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpScalarFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWarpScalarFilter, argc, argv);
}
