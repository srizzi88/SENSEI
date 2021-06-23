//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/arg/FetchTagArrayDirectInOut.h>

#include <svtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <svtkm/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

static svtkm::Id g_NumSets;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    SVTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    SVTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }

  SVTKM_EXEC_CONT
  void Set(svtkm::Id index, const ValueType& value) const
  {
    SVTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    SVTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    SVTKM_TEST_ASSERT(test_equal(value, ValueType(2) * TestValue(index, ValueType())),
                     "Tried to set invalid value.");
    g_NumSets++;
  }
};

template <typename T>
struct FetchArrayDirectInTests
{

  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayDirectInOut,
                                             svtkm::exec::arg::AspectTagDefault,
                                             svtkm::exec::arg::ThreadIndicesTesting,
                                             TestPortal<T>>;

    FetchType fetch;

    g_NumSets = 0;

    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      svtkm::exec::arg::ThreadIndicesTesting indices(index);

      T value = fetch.Load(indices, execObject);
      SVTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())), "Got invalid value from Load.");

      value = T(T(2) * value);

      fetch.Store(indices, execObject, value);
    }

    SVTKM_TEST_ASSERT(g_NumSets == ARRAY_SIZE,
                     "Array portal's set not called correct number of times."
                     "Store method must be wrong.");
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayDirectInTests<T>()();
  }
};

void TestExecObjectFetch()
{
  svtkm::testing::Testing::TryTypes(TryType(), svtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestFetchArrayDirectInOut(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
