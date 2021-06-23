//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TypeCheckTagKeys.h>

#include <svtkm/worklet/Keys.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

struct TestNotKeys
{
};

void TestCheckKeys()
{
  std::cout << "Checking reporting of type checking keys." << std::endl;

  using svtkm::cont::arg::TypeCheck;
  using svtkm::cont::arg::TypeCheckTagKeys;

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, svtkm::worklet::Keys<svtkm::Id>>::value),
                   "Type check failed.");
  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, svtkm::worklet::Keys<svtkm::Float32>>::value),
                   "Type check failed.");
  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, svtkm::worklet::Keys<svtkm::Id3>>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, TestNotKeys>::value), "Type check failed.");
  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, svtkm::Id>::value), "Type check failed.");
  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, svtkm::cont::ArrayHandle<svtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckKeys(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCheckKeys, argc, argv);
}
