//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/arg/WorkIndex.h>

#include <svtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <svtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <svtkm/testing/Testing.h>

namespace
{

void TestWorkIndexFetch()
{
  std::cout << "Trying WorkIndex fetch." << std::endl;

  using FetchType =
    svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayDirectIn, // Not used but probably common.
                           svtkm::exec::arg::AspectTagWorkIndex,
                           svtkm::exec::arg::ThreadIndicesTesting,
                           svtkm::internal::NullType>;

  FetchType fetch;

  for (svtkm::Id index = 0; index < 10; index++)
  {
    svtkm::exec::arg::ThreadIndicesTesting indices(index);

    svtkm::Id value = fetch.Load(indices, svtkm::internal::NullType());
    SVTKM_TEST_ASSERT(value == index, "Fetch did not give correct work index.");

    value++;

    // This should be a no-op.
    fetch.Store(indices, svtkm::internal::NullType(), value);
  }
}

} // anonymous namespace

int UnitTestFetchWorkIndex(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestWorkIndexFetch, argc, argv);
}
