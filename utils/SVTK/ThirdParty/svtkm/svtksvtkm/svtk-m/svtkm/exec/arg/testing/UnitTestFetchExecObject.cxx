//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/arg/FetchTagExecObject.h>

#include <svtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <svtkm/testing/Testing.h>

#define EXPECTED_NUMBER 67

namespace
{

struct TestExecutionObject
{
  TestExecutionObject()
    : Number(static_cast<svtkm::Int32>(0xDEADDEAD))
  {
  }
  TestExecutionObject(svtkm::Int32 number)
    : Number(number)
  {
  }
  svtkm::Int32 Number;
};

void TryInvocation()
{
  TestExecutionObject execObjectStore(EXPECTED_NUMBER);

  using FetchType = svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagExecObject,
                                           svtkm::exec::arg::AspectTagDefault,
                                           svtkm::exec::arg::ThreadIndicesTesting,
                                           TestExecutionObject>;

  FetchType fetch;

  svtkm::exec::arg::ThreadIndicesTesting indices(0);

  TestExecutionObject execObject = fetch.Load(indices, execObjectStore);
  SVTKM_TEST_ASSERT(execObject.Number == EXPECTED_NUMBER, "Did not load object correctly.");

  execObject.Number = -1;

  // This should be a no-op.
  fetch.Store(indices, execObjectStore, execObject);

  // Data in Invocation should not have changed.
  SVTKM_TEST_ASSERT(execObjectStore.Number == EXPECTED_NUMBER,
                   "Fetch changed read-only execution object.");
}

void TestExecObjectFetch()
{
  TryInvocation();
}

} // anonymous namespace

int UnitTestFetchExecObject(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
