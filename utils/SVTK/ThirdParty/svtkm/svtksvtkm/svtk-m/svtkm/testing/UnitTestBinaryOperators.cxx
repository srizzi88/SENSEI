//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/BinaryOperators.h>

#include <svtkm/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryOperatorTest()
{

  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Sum
  {
    svtkm::Sum sum;
    T result;

    result = sum(svtkm::TypeTraits<T>::ZeroInitialization(), T(1));
    SVTKM_TEST_ASSERT(result == T(1), "Sum wrong.");

    result = sum(T(1), T(1));
    SVTKM_TEST_ASSERT(result == T(2), "Sum wrong.");
  }

  //test Product
  {
    svtkm::Product product;
    T result;

    result = product(svtkm::TypeTraits<T>::ZeroInitialization(), T(1));
    SVTKM_TEST_ASSERT(result == svtkm::TypeTraits<T>::ZeroInitialization(), "Product wrong.");

    result = product(T(1), T(1));
    SVTKM_TEST_ASSERT(result == T(1), "Product wrong.");

    result = product(T(2), T(3));
    SVTKM_TEST_ASSERT(result == T(6), "Product wrong.");
  }

  //test Maximum
  {
    svtkm::Maximum maximum;
    SVTKM_TEST_ASSERT(maximum(T(1), T(2)) == T(2), "Maximum wrong.");
    SVTKM_TEST_ASSERT(maximum(T(2), T(2)) == T(2), "Maximum wrong.");
    SVTKM_TEST_ASSERT(maximum(T(2), T(1)) == T(2), "Maximum wrong.");
  }

  //test Minimum
  {
    svtkm::Minimum minimum;
    SVTKM_TEST_ASSERT(minimum(T(1), T(2)) == T(1), "Minimum wrong.");
    SVTKM_TEST_ASSERT(minimum(T(1), T(1)) == T(1), "Minimum wrong.");
    SVTKM_TEST_ASSERT(minimum(T(3), T(2)) == T(2), "Minimum wrong.");
  }

  //test MinAndMax
  {
    svtkm::MinAndMax<T> min_and_max;
    svtkm::Vec<T, 2> result;

    // Test1: basic param
    {
      result = min_and_max(T(1));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(1), T(1))), "Test1 MinAndMax wrong");
    }

    // Test2: basic param
    {
      result = min_and_max(svtkm::TypeTraits<T>::ZeroInitialization(), T(1));
      SVTKM_TEST_ASSERT(
        test_equal(result, svtkm::Vec<T, 2>(svtkm::TypeTraits<T>::ZeroInitialization(), T(1))),
        "Test2 MinAndMax wrong");

      result = min_and_max(T(2), T(1));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(1), T(2))), "Test2 MinAndMax wrong");
    }

    // Test3: 1st param vector, 2nd param basic
    {
      result = min_and_max(svtkm::Vec<T, 2>(3, 5), T(7));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(3), T(7))), "Test3 MinAndMax Wrong");

      result = min_and_max(svtkm::Vec<T, 2>(3, 5), T(2));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(2), T(5))), "Test3 MinAndMax Wrong");
    }

    // Test4: 1st param basic, 2nd param vector
    {
      result = min_and_max(T(7), svtkm::Vec<T, 2>(3, 5));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(3), T(7))), "Test4 MinAndMax Wrong");

      result = min_and_max(T(2), svtkm::Vec<T, 2>(3, 5));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(2), T(5))), "Test4 MinAndMax Wrong");
    }

    // Test5: 2 vector param
    {
      result = min_and_max(svtkm::Vec<T, 2>(2, 4), svtkm::Vec<T, 2>(3, 5));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(2), T(5))), "Test5 MinAndMax Wrong");

      result = min_and_max(svtkm::Vec<T, 2>(2, 7), svtkm::Vec<T, 2>(3, 5));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(2), T(7))), "Test5 MinAndMax Wrong");

      result = min_and_max(svtkm::Vec<T, 2>(4, 4), svtkm::Vec<T, 2>(1, 8));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(1), T(8))), "Test5 MinAndMax Wrong");

      result = min_and_max(svtkm::Vec<T, 2>(4, 4), svtkm::Vec<T, 2>(3, 3));
      SVTKM_TEST_ASSERT(test_equal(result, svtkm::Vec<T, 2>(T(3), T(4))), "Test5 MinAndMax Wrong");
    }
  }
}

struct BinaryOperatorTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    BinaryOperatorTest<T>();
  }
};

void TestBinaryOperators()
{
  svtkm::testing::Testing::TryTypes(BinaryOperatorTestFunctor());

  svtkm::UInt32 v1 = 0xccccccccu;
  svtkm::UInt32 v2 = 0xffffffffu;
  svtkm::UInt32 v3 = 0x0u;

  //test BitwiseAnd
  {
    svtkm::BitwiseAnd bitwise_and;
    SVTKM_TEST_ASSERT(bitwise_and(v1, v2) == (v1 & v2), "bitwise_and wrong.");
    SVTKM_TEST_ASSERT(bitwise_and(v1, v3) == (v1 & v3), "bitwise_and wrong.");
    SVTKM_TEST_ASSERT(bitwise_and(v2, v3) == (v2 & v3), "bitwise_and wrong.");
  }

  //test BitwiseOr
  {
    svtkm::BitwiseOr bitwise_or;
    SVTKM_TEST_ASSERT(bitwise_or(v1, v2) == (v1 | v2), "bitwise_or wrong.");
    SVTKM_TEST_ASSERT(bitwise_or(v1, v3) == (v1 | v3), "bitwise_or wrong.");
    SVTKM_TEST_ASSERT(bitwise_or(v2, v3) == (v2 | v3), "bitwise_or wrong.");
  }

  //test BitwiseXor
  {
    svtkm::BitwiseXor bitwise_xor;
    SVTKM_TEST_ASSERT(bitwise_xor(v1, v2) == (v1 ^ v2), "bitwise_xor wrong.");
    SVTKM_TEST_ASSERT(bitwise_xor(v1, v3) == (v1 ^ v3), "bitwise_xor wrong.");
    SVTKM_TEST_ASSERT(bitwise_xor(v2, v3) == (v2 ^ v3), "bitwise_xor wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryOperators(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestBinaryOperators, argc, argv);
}
