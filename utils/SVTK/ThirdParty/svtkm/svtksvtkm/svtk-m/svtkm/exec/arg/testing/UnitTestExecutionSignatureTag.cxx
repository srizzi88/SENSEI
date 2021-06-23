//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/arg/BasicArg.h>
#include <svtkm/exec/arg/WorkIndex.h>

#include <svtkm/testing/Testing.h>

namespace
{

void TestExecutionSignatures()
{
  SVTKM_IS_EXECUTION_SIGNATURE_TAG(svtkm::exec::arg::BasicArg<1>);

  SVTKM_TEST_ASSERT(
    svtkm::exec::arg::internal::ExecutionSignatureTagCheck<svtkm::exec::arg::BasicArg<2>>::Valid,
    "Bad check for BasicArg");

  SVTKM_TEST_ASSERT(
    svtkm::exec::arg::internal::ExecutionSignatureTagCheck<svtkm::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  SVTKM_TEST_ASSERT(!svtkm::exec::arg::internal::ExecutionSignatureTagCheck<svtkm::Id>::Valid,
                   "Bad check for svtkm::Id");
}

} // anonymous namespace

int UnitTestExecutionSignatureTag(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestExecutionSignatures, argc, argv);
}
