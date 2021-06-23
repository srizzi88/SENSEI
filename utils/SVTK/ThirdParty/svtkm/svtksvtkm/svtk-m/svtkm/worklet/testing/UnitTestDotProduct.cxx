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
#include <svtkm/worklet/DotProduct.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
T normalizedVector(T v)
{
  T vN = svtkm::Normal(v);
  return vN;
}

template <typename T>
void createVectors(std::vector<svtkm::Vec<T, 3>>& vecs1,
                   std::vector<svtkm::Vec<T, 3>>& vecs2,
                   std::vector<T>& result)
{
  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  result.push_back(1);

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(-1), T(0), T(0))));
  result.push_back(-1);

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(0), T(1), T(0))));
  result.push_back(0);

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(0), T(-1), T(0))));
  result.push_back(0);

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(T(1.0 / svtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(1), T(1), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(1), T(0), T(0))));
  result.push_back(T(1.0 / svtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(-1), T(0), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(-T(1.0 / svtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(svtkm::make_Vec(T(0), T(1), T(0))));
  vecs2.push_back(normalizedVector(svtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(T(1.0 / svtkm::Sqrt(2.0)));
}

template <typename T>
void TestDotProduct()
{
  std::vector<svtkm::Vec<T, 3>> inputVecs1, inputVecs2;
  std::vector<T> answer;
  createVectors(inputVecs1, inputVecs2, answer);

  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> inputArray1, inputArray2;
  svtkm::cont::ArrayHandle<T> outputArray;
  inputArray1 = svtkm::cont::make_ArrayHandle(inputVecs1);
  inputArray2 = svtkm::cont::make_ArrayHandle(inputVecs2);

  svtkm::worklet::DotProduct dotProductWorklet;
  svtkm::worklet::DispatcherMapField<svtkm::worklet::DotProduct> dispatcherDotProduct(
    dotProductWorklet);
  dispatcherDotProduct.Invoke(inputArray1, inputArray2, outputArray);

  SVTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray1.GetNumberOfValues(),
                   "Wrong number of results for DotProduct worklet");

  for (svtkm::Id i = 0; i < inputArray1.GetNumberOfValues(); i++)
  {
    svtkm::Vec<T, 3> v1 = inputArray1.GetPortalConstControl().Get(i);
    svtkm::Vec<T, 3> v2 = inputArray2.GetPortalConstControl().Get(i);
    T ans = answer[static_cast<std::size_t>(i)];

    SVTKM_TEST_ASSERT(test_equal(ans, svtkm::Dot(v1, v2)), "Wrong result for dot product");
  }
}

void TestDotProductWorklets()
{
  std::cout << "Testing DotProduct Worklet" << std::endl;
  TestDotProduct<svtkm::Float32>();
  //  TestDotProduct<svtkm::Float64>();
}
}

int UnitTestDotProduct(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestDotProductWorklets, argc, argv);
}
