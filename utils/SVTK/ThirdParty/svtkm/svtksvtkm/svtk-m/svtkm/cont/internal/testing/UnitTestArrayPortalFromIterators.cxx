//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/internal/ArrayPortalFromIterators.h>

#include <svtkm/VecTraits.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr svtkm::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;

  ValueType ExpectedValue(svtkm::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<svtkm::Id>(value)));
  }

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    svtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      *iter = ExpectedValue(index, value);
      index++;
    }
  }

  template <class IteratorType>
  bool CheckIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    svtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      if (*iter != ExpectedValue(index, value))
      {
        return false;
      }
      index++;
    }
    return true;
  }

  template <class PortalType>
  bool CheckPortal(const PortalType& portal, ComponentType value)
  {
    svtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
    return CheckIterator(iterators.GetBegin(), iterators.GetEnd(), value);
  }

  void operator()()
  {
    ValueType array[ARRAY_SIZE];

    constexpr ComponentType ORIGINAL_VALUE = 109;
    FillIterator(array, array + ARRAY_SIZE, ORIGINAL_VALUE);

    ::svtkm::cont::internal::ArrayPortalFromIterators<ValueType*> portal(array, array + ARRAY_SIZE);
    ::svtkm::cont::internal::ArrayPortalFromIterators<const ValueType*> const_portal(
      array, array + ARRAY_SIZE);

    std::cout << "  Check that ArrayPortalToIterators is not doing indirection." << std::endl;
    // If you get a compile error here about mismatched types, it might be
    // that ArrayPortalToIterators is not properly overloaded to return the
    // original iterator.
    SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorBegin(portal) == array,
                     "Begin iterator wrong.");
    SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorEnd(portal) == array + ARRAY_SIZE,
                     "End iterator wrong.");
    SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorBegin(const_portal) == array,
                     "Begin const iterator wrong.");
    SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorEnd(const_portal) == array + ARRAY_SIZE,
                     "End const iterator wrong.");

    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE, "Portal array size wrong.");
    SVTKM_TEST_ASSERT(const_portal.GetNumberOfValues() == ARRAY_SIZE,
                     "Const portal array size wrong.");

    std::cout << "  Check initial value." << std::endl;
    SVTKM_TEST_ASSERT(CheckPortal(portal, ORIGINAL_VALUE), "Portal iterator has bad value.");
    SVTKM_TEST_ASSERT(CheckPortal(const_portal, ORIGINAL_VALUE),
                     "Const portal iterator has bad value.");

    constexpr ComponentType SET_VALUE = 62;

    std::cout << "  Check get/set methods." << std::endl;
    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      SVTKM_TEST_ASSERT(portal.Get(index) == ExpectedValue(index, ORIGINAL_VALUE),
                       "Bad portal value.");
      SVTKM_TEST_ASSERT(const_portal.Get(index) == ExpectedValue(index, ORIGINAL_VALUE),
                       "Bad const portal value.");

      portal.Set(index, ExpectedValue(index, SET_VALUE));
    }

    std::cout << "  Make sure set has correct value." << std::endl;
    SVTKM_TEST_ASSERT(CheckPortal(portal, SET_VALUE), "Portal iterator has bad value.");
    SVTKM_TEST_ASSERT(CheckIterator(array, array + ARRAY_SIZE, SET_VALUE), "Array has bad value.");
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayPortalFromIterators()
{
  svtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayPortalFromIterators(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayPortalFromIterators, argc, argv);
}
