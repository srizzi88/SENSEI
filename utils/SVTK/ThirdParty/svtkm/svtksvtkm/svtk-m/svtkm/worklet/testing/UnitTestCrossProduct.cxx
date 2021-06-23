//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/CrossProduct.h>
#include <svtkm/worklet/DispatcherMapField.h>

#include <random>
#include <svtkm/cont/testing/Testing.h>

namespace
{
std::mt19937 randGenerator;

template <typename T>
void createVectors(std::vector<svtkm::Vec<T, 3>>& vecs1, std::vector<svtkm::Vec<T, 3>>& vecs2)
{
  // First, test the standard directions.
  // X x Y
  vecs1.push_back(svtkm::make_Vec(1, 0, 0));
  vecs2.push_back(svtkm::make_Vec(0, 1, 0));

  // Y x Z
  vecs1.push_back(svtkm::make_Vec(0, 1, 0));
  vecs2.push_back(svtkm::make_Vec(0, 0, 1));

  // Z x X
  vecs1.push_back(svtkm::make_Vec(0, 0, 1));
  vecs2.push_back(svtkm::make_Vec(1, 0, 0));

  // Y x X
  vecs1.push_back(svtkm::make_Vec(0, 1, 0));
  vecs2.push_back(svtkm::make_Vec(1, 0, 0));

  // Z x Y
  vecs1.push_back(svtkm::make_Vec(0, 0, 1));
  vecs2.push_back(svtkm::make_Vec(0, 1, 0));

  // X x Z
  vecs1.push_back(svtkm::make_Vec(1, 0, 0));
  vecs2.push_back(svtkm::make_Vec(0, 0, 1));

  //Test some other vector combinations
  std::uniform_real_distribution<svtkm::Float64> randomDist(-10.0, 10.0);
  randomDist(randGenerator);

  for (int i = 0; i < 100; i++)
  {
    vecs1.push_back(svtkm::make_Vec(
      randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator)));
    vecs2.push_back(svtkm::make_Vec(
      randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator)));
  }
}

template <typename T>
void TestCrossProduct()
{
  std::vector<svtkm::Vec<T, 3>> inputVecs1, inputVecs2;
  createVectors(inputVecs1, inputVecs2);

  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> inputArray1, inputArray2;
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> outputArray;
  inputArray1 = svtkm::cont::make_ArrayHandle(inputVecs1);
  inputArray2 = svtkm::cont::make_ArrayHandle(inputVecs2);

  svtkm::worklet::CrossProduct crossProductWorklet;
  svtkm::worklet::DispatcherMapField<svtkm::worklet::CrossProduct> dispatcherCrossProduct(
    crossProductWorklet);
  dispatcherCrossProduct.Invoke(inputArray1, inputArray2, outputArray);

  SVTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray1.GetNumberOfValues(),
                   "Wrong number of results for CrossProduct worklet");

  //Test the canonical cases.
  SVTKM_TEST_ASSERT(
    test_equal(outputArray.GetPortalConstControl().Get(0), svtkm::make_Vec(0, 0, 1)) &&
      test_equal(outputArray.GetPortalConstControl().Get(1), svtkm::make_Vec(1, 0, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(2), svtkm::make_Vec(0, 1, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(3), svtkm::make_Vec(0, 0, -1)) &&
      test_equal(outputArray.GetPortalConstControl().Get(4), svtkm::make_Vec(-1, 0, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(5), svtkm::make_Vec(0, -1, 0)),
    "Wrong result for CrossProduct worklet");

  for (svtkm::Id i = 0; i < inputArray1.GetNumberOfValues(); i++)
  {
    svtkm::Vec<T, 3> v1 = inputArray1.GetPortalConstControl().Get(i);
    svtkm::Vec<T, 3> v2 = inputArray2.GetPortalConstControl().Get(i);
    svtkm::Vec<T, 3> res = outputArray.GetPortalConstControl().Get(i);

    //Make sure result is orthogonal each input vector. Need to normalize to compare with zero.
    svtkm::Vec<T, 3> v1N(svtkm::Normal(v1)), v2N(svtkm::Normal(v1)), resN(svtkm::Normal(res));
    SVTKM_TEST_ASSERT(test_equal(svtkm::Dot(resN, v1N), T(0.0)), "Wrong result for cross product");
    SVTKM_TEST_ASSERT(test_equal(svtkm::Dot(resN, v2N), T(0.0)), "Wrong result for cross product");

    T sinAngle = svtkm::Magnitude(res) * svtkm::RMagnitude(v1) * svtkm::RMagnitude(v2);
    T cosAngle = svtkm::Dot(v1, v2) * svtkm::RMagnitude(v1) * svtkm::RMagnitude(v2);
    SVTKM_TEST_ASSERT(test_equal(sinAngle * sinAngle + cosAngle * cosAngle, T(1.0)),
                     "Bad cross product length.");
  }
}

void TestCrossProductWorklets()
{
  std::cout << "Testing CrossProduct Worklet" << std::endl;
  TestCrossProduct<svtkm::Float32>();
  TestCrossProduct<svtkm::Float64>();
}
}

int UnitTestCrossProduct(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCrossProductWorklets, argc, argv);
}
