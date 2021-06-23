//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayPortalToIterators.h>

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

  static ValueType ExpectedValue(svtkm::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<svtkm::Id>(value)));
  }

  class ReadOnlyArrayPortal
  {
  public:
    using ValueType = T;

    SVTKM_CONT
    ReadOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    SVTKM_CONT
    svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    SVTKM_CONT
    ValueType Get(svtkm::Id index) const { return ExpectedValue(index, this->Value); }

  private:
    ComponentType Value;
  };

  class WriteOnlyArrayPortal
  {
  public:
    using ValueType = T;

    SVTKM_CONT
    WriteOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    SVTKM_CONT
    svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    SVTKM_CONT
    void Set(svtkm::Id index, const ValueType& value) const
    {
      SVTKM_TEST_ASSERT(value == ExpectedValue(index, this->Value),
                       "Set unexpected value in array portal.");
    }

  private:
    ComponentType Value;
  };

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    std::cout << "    Check distance" << std::endl;
    SVTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                     "Distance between begin and end incorrect.");

    std::cout << "    Write expected value in iterator." << std::endl;
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
    std::cout << "    Check distance" << std::endl;
    SVTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                     "Distance between begin and end incorrect.");

    std::cout << "    Read expected value from iterator." << std::endl;
    svtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      SVTKM_TEST_ASSERT(ValueType(*iter) == ExpectedValue(index, value),
                       "Got bad value from iterator.");
      index++;
    }
    return true;
  }

  void TestIteratorRead()
  {
    using ArrayPortalType = ReadOnlyArrayPortal;
    using GetIteratorsType = svtkm::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType READ_VALUE = 23;
    ArrayPortalType portal(READ_VALUE);

    std::cout << "  Testing read-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    CheckIterator(iterators.GetBegin(), iterators.GetEnd(), READ_VALUE);

    std::cout << "  Testing read-only iterators with convenience functions." << std::endl;
    CheckIterator(svtkm::cont::ArrayPortalToIteratorBegin(portal),
                  svtkm::cont::ArrayPortalToIteratorEnd(portal),
                  READ_VALUE);
  }

  void TestIteratorWrite()
  {
    using ArrayPortalType = WriteOnlyArrayPortal;
    using GetIteratorsType = svtkm::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType WRITE_VALUE = 63;
    ArrayPortalType portal(WRITE_VALUE);

    std::cout << "  Testing write-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    FillIterator(iterators.GetBegin(), iterators.GetEnd(), WRITE_VALUE);

    std::cout << "  Testing write-only iterators with convenience functions." << std::endl;
    FillIterator(svtkm::cont::ArrayPortalToIteratorBegin(portal),
                 svtkm::cont::ArrayPortalToIteratorEnd(portal),
                 WRITE_VALUE);
  }

  void operator()()
  {
    TestIteratorRead();
    TestIteratorWrite();
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

// Defines minimal API needed for ArrayPortalToIterators to detect and
// use custom iterators:
struct SpecializedIteratorAPITestPortal
{
  using IteratorType = int;
  IteratorType GetIteratorBegin() const { return 32; }
  IteratorType GetIteratorEnd() const { return 13; }
};

void TestCustomIterator()
{
  std::cout << "  Testing custom iterator detection." << std::endl;

  // Dummy portal type for this test:
  using PortalType = SpecializedIteratorAPITestPortal;
  using ItersType = svtkm::cont::ArrayPortalToIterators<PortalType>;

  PortalType portal;
  ItersType iters{ portal };

  SVTKM_TEST_ASSERT(
    std::is_same<typename ItersType::IteratorType, typename PortalType::IteratorType>::value);
  SVTKM_TEST_ASSERT(
    std::is_same<decltype(iters.GetBegin()), typename PortalType::IteratorType>::value);
  SVTKM_TEST_ASSERT(
    std::is_same<decltype(iters.GetEnd()), typename PortalType::IteratorType>::value);
  SVTKM_TEST_ASSERT(iters.GetBegin() == 32);
  SVTKM_TEST_ASSERT(iters.GetEnd() == 13);

  // Convenience API, too:
  SVTKM_TEST_ASSERT(std::is_same<decltype(svtkm::cont::ArrayPortalToIteratorBegin(portal)),
                                typename PortalType::IteratorType>::value);
  SVTKM_TEST_ASSERT(std::is_same<decltype(svtkm::cont::ArrayPortalToIteratorEnd(portal)),
                                typename PortalType::IteratorType>::value);
  SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorBegin(portal) == 32);
  SVTKM_TEST_ASSERT(svtkm::cont::ArrayPortalToIteratorEnd(portal) == 13);
}

void TestBasicStorageSpecialization()
{
  // Control iterators from basic storage arrays should just be pointers:
  svtkm::cont::ArrayHandle<int> handle;
  handle.Allocate(1);

  auto portal = handle.GetPortalControl();
  auto portalConst = handle.GetPortalConstControl();

  auto iter = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto iterConst = svtkm::cont::ArrayPortalToIteratorBegin(portalConst);

  (void)iter;
  (void)iterConst;

  SVTKM_TEST_ASSERT(std::is_same<decltype(iter), int*>::value);
  SVTKM_TEST_ASSERT(std::is_same<decltype(iterConst), int const*>::value);
}

void TestArrayPortalToIterators()
{
  svtkm::testing::Testing::TryTypes(TestFunctor());
  TestCustomIterator();
  TestBasicStorageSpecialization();
}

} // Anonymous namespace

int UnitTestArrayPortalToIterators(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayPortalToIterators, argc, argv);
}
