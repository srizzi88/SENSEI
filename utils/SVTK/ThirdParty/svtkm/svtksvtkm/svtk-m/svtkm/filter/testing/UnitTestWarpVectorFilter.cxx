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
#include <svtkm/filter/WarpVector.h>

#include <random>
#include <vector>

namespace
{
const svtkm::Id dim = 5;
template <typename T>
svtkm::cont::DataSet MakeWarpVectorTestDataSet()
{
  using vecType = svtkm::Vec<T, 3>;
  svtkm::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  for (svtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (svtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(svtkm::make_Vec(x, y, z));
      vec1.push_back(svtkm::make_Vec(x, y, y));
    }
  }

  dataSet.AddCoordinateSystem(
    svtkm::cont::make_CoordinateSystem("coordinates", coordinates, svtkm::CopyFlag::On));

  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec1", vec1);

  vecType vector = svtkm::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(2.0));
  svtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    svtkm::cont::make_ArrayHandleConstant(vector, dim * dim);
  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec2", vectorAH);

  return dataSet;
}

class PolicyWarpVector : public svtkm::filter::PolicyBase<PolicyWarpVector>
{
public:
  using vecType = svtkm::Vec3f;
  using TypeListWarpVectorTags = svtkm::List<svtkm::cont::ArrayHandleConstant<vecType>::StorageTag,
                                            svtkm::cont::ArrayHandle<vecType>::StorageTag>;
  using FieldStorageList = TypeListWarpVectorTags;
};

void CheckResult(const svtkm::filter::WarpVector& filter, const svtkm::cont::DataSet& result)
{
  SVTKM_TEST_ASSERT(result.HasPointField("warpvector"), "Output filed WarpVector is missing");
  using vecType = svtkm::Vec3f;
  svtkm::cont::ArrayHandle<vecType> outputArray;
  result.GetPointField("warpvector").GetData().CopyTo(outputArray);
  auto outPortal = outputArray.GetPortalConstControl();

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
        ? z + static_cast<svtkm::FloatDefault>(2 * 2)
        : y + static_cast<svtkm::FloatDefault>(2 * 2);
      auto point = outPortal.Get(j * dim + i);
      SVTKM_TEST_ASSERT(point[0] == x, "Wrong result of x value for warp vector");
      SVTKM_TEST_ASSERT(point[1] == y, "Wrong result of y value for warp vector");
      SVTKM_TEST_ASSERT(point[2] == targetZ, "Wrong result of z value for warp vector");
    }
  }
  SVTKM_TEST_ASSERT(filter.GetVectorFieldName() == "vec2", "Vector field name is wrong");
}

void TestWarpVectorFilter()
{
  std::cout << "Testing WarpVector filter" << std::endl;
  svtkm::cont::DataSet ds = MakeWarpVectorTestDataSet<svtkm::FloatDefault>();
  svtkm::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    svtkm::filter::WarpVector filter(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetVectorField("vec2");
    svtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpVector());
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    svtkm::filter::WarpVector filter(scale);
    filter.SetActiveField("vec1");
    filter.SetVectorField("vec2");
    svtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpVector());
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpVectorFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWarpVectorFilter, argc, argv);
}
