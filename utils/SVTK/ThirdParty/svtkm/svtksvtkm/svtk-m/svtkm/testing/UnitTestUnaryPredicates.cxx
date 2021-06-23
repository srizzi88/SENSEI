//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/UnaryPredicates.h>

#include <svtkm/testing/Testing.h>

namespace
{

template <typename T>
void UnaryPredicateTest()
{
  //test IsZeroInitialized
  {
    svtkm::IsZeroInitialized is_default;
    SVTKM_TEST_ASSERT(is_default(svtkm::TypeTraits<T>::ZeroInitialization()) == true,
                     "IsZeroInitialized wrong.");
    SVTKM_TEST_ASSERT(is_default(TestValue(1, T())) == false, "IsZeroInitialized wrong.");
  }

  //test NotZeroInitialized
  {
    svtkm::NotZeroInitialized not_default;
    SVTKM_TEST_ASSERT(not_default(svtkm::TypeTraits<T>::ZeroInitialization()) == false,
                     "NotZeroInitialized wrong.");
    SVTKM_TEST_ASSERT(not_default(TestValue(1, T())) == true, "NotZeroInitialized wrong.");
  }
}

struct UnaryPredicateTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    UnaryPredicateTest<T>();
  }
};

void TestUnaryPredicates()
{
  svtkm::testing::Testing::TryTypes(UnaryPredicateTestFunctor());

  //test LogicalNot
  {
    svtkm::LogicalNot logical_not;
    SVTKM_TEST_ASSERT(logical_not(true) == false, "logical_not true wrong.");
    SVTKM_TEST_ASSERT(logical_not(false) == true, "logical_not false wrong.");
  }
}

} // anonymous namespace

int UnitTestUnaryPredicates(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestUnaryPredicates, argc, argv);
}
