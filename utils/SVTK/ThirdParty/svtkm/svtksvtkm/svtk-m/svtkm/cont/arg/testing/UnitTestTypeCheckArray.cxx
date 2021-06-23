//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TypeCheckTagArray.h>
#include <svtkm/cont/arg/TypeCheckTagAtomicArray.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleCounting.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

struct TryArraysOfType
{
  template <typename T>
  void operator()(T) const
  {
    using svtkm::cont::arg::TypeCheck;
    using svtkm::cont::arg::TypeCheckTagArray;

    using StandardArray = svtkm::cont::ArrayHandle<T>;
    SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, StandardArray>::value),
                     "Standard array type check failed.");

    using CountingArray = svtkm::cont::ArrayHandleCounting<T>;
    SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, CountingArray>::value),
                     "Counting array type check failed.");

    using CompositeArray = svtkm::cont::ArrayHandleCompositeVector<StandardArray, CountingArray>;
    SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagArray, CompositeArray>::value),
                     "Composite array type check failed.");

    // Just some type that is not a valid array.
    using NotAnArray = typename StandardArray::PortalControl;
    SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray, NotAnArray>::value),
                     "Not an array type check failed.");

    // Another type that is not a valid array.
    SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagArray, T>::value), "Not an array type check failed.");
  }
};

void TestCheckAtomicArray()
{
  std::cout << "Trying some arrays with atomic arrays." << std::endl;
  using svtkm::cont::arg::TypeCheck;
  using svtkm::cont::arg::TypeCheckTagAtomicArray;

  using Int32Array = svtkm::cont::ArrayHandle<svtkm::Int32>;
  using Int64Array = svtkm::cont::ArrayHandle<svtkm::Int64>;
  using FloatArray = svtkm::cont::ArrayHandle<svtkm::Float32>;

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int32Array>::value),
                   "Check for 32-bit int failed.");
  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagAtomicArray, Int64Array>::value),
                   "Check for 64-bit int failed.");
  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagAtomicArray, FloatArray>::value),
                   "Check for float failed.");
}

void TestCheckArray()
{
  svtkm::testing::Testing::TryTypes(TryArraysOfType());

  TestCheckAtomicArray();
}

} // anonymous namespace

int UnitTestTypeCheckArray(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCheckArray, argc, argv);
}
