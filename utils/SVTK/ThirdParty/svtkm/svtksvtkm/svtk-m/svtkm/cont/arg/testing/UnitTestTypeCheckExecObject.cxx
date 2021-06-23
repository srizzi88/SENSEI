//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TypeCheckTagExecObject.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

struct TestExecutionObject : svtkm::cont::ExecutionObjectBase
{
};
struct TestNotExecutionObject
{
};

void TestCheckExecObject()
{
  std::cout << "Checking reporting of type checking exec object." << std::endl;

  using svtkm::cont::arg::TypeCheck;
  using svtkm::cont::arg::TypeCheckTagExecObject;

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagExecObject, TestExecutionObject>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, TestNotExecutionObject>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, svtkm::Id>::value), "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, svtkm::cont::ArrayHandle<svtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckExecObject(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCheckExecObject, argc, argv);
}
