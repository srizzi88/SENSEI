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
#include <svtkm/filter/VectorMagnitude.h>

#include <vector>

namespace
{

void TestVectorMagnitude()
{
  std::cout << "Testing VectorMagnitude Filter" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  const int nVerts = 18;
  svtkm::Float64 fvars[nVerts] = { 10.1,  20.1,  30.1,  40.1,  50.2,  60.2,  70.2,  80.2,  90.3,
                                  100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5 };

  std::vector<svtkm::Vec3f_64> fvec(nVerts);
  for (std::size_t i = 0; i < fvec.size(); ++i)
  {
    fvec[i] = svtkm::make_Vec(fvars[i], fvars[i], fvars[i]);
  }
  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> finput = svtkm::cont::make_ArrayHandle(fvec);

  svtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "double_vec_pointvar", finput);

  svtkm::filter::VectorMagnitude vm;
  vm.SetActiveField("double_vec_pointvar");
  auto result = vm.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasPointField("magnitude"), "Output field missing.");

  svtkm::cont::ArrayHandle<svtkm::Float64> resultArrayHandle;
  result.GetPointField("magnitude").GetData().CopyTo(resultArrayHandle);
  for (svtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(std::sqrt(3 * fvars[i] * fvars[i]),
                                resultArrayHandle.GetPortalConstControl().Get(i)),
                     "Wrong result for Magnitude worklet");
  }
}
}

int UnitTestVectorMagnitudeFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestVectorMagnitude, argc, argv);
}
