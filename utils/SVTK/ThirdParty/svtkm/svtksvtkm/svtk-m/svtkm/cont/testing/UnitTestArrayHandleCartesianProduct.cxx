//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace ArrayHandleCartesianProductNamespace
{

template <typename T>
void ArrayHandleCPBasic(svtkm::cont::ArrayHandle<T> x,
                        svtkm::cont::ArrayHandle<T> y,
                        svtkm::cont::ArrayHandle<T> z)

{
  svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<T>,
                                          svtkm::cont::ArrayHandle<T>,
                                          svtkm::cont::ArrayHandle<T>>
    cpArray;

  svtkm::Id nx = x.GetNumberOfValues();
  svtkm::Id ny = y.GetNumberOfValues();
  svtkm::Id nz = z.GetNumberOfValues();
  svtkm::Id n = nx * ny * nz;

  cpArray = svtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);

  //Make sure we have the right number of values.
  SVTKM_TEST_ASSERT(cpArray.GetNumberOfValues() == (nx * ny * nz),
                   "Cartesian array constructor has wrong number of values");

  //Make sure the values are correct.
  svtkm::Vec<T, 3> val;
  for (svtkm::Id i = 0; i < n; i++)
  {
    svtkm::Id idx0 = (i % (nx * ny)) % nx;
    svtkm::Id idx1 = (i % (nx * ny)) / nx;
    svtkm::Id idx2 = i / (nx * ny);

    val = svtkm::Vec<T, 3>(x.GetPortalConstControl().Get(idx0),
                          y.GetPortalConstControl().Get(idx1),
                          z.GetPortalConstControl().Get(idx2));
    SVTKM_TEST_ASSERT(test_equal(cpArray.GetPortalConstControl().Get(i), val),
                     "Wrong value in array");
  }
}

template <typename T>
void createArr(std::vector<T>& arr, std::size_t n)
{
  arr.resize(n);
  for (std::size_t i = 0; i < n; i++)
    arr[i] = static_cast<T>(i);
}

template <typename T>
void RunTest()
{
  std::size_t nX = 11, nY = 13, nZ = 11;

  for (std::size_t i = 1; i < nX; i += 2)
  {
    for (std::size_t j = 1; j < nY; j += 4)
    {
      for (std::size_t k = 1; k < nZ; k += 5)
      {
        std::vector<T> X, Y, Z;
        createArr(X, nX);
        createArr(Y, nY);
        createArr(Z, nZ);

        ArrayHandleCPBasic(svtkm::cont::make_ArrayHandle(X),
                           svtkm::cont::make_ArrayHandle(Y),
                           svtkm::cont::make_ArrayHandle(Z));
      }
    }
  }
}

void TestArrayHandleCartesianProduct()
{
  RunTest<svtkm::Float32>();
  RunTest<svtkm::Float64>();
  RunTest<svtkm::Id>();
}

} // namespace ArrayHandleCartesianProductNamespace

int UnitTestArrayHandleCartesianProduct(int argc, char* argv[])
{
  using namespace ArrayHandleCartesianProductNamespace;
  return svtkm::cont::testing::Testing::Run(TestArrayHandleCartesianProduct, argc, argv);
}
