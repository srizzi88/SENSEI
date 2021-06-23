//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This meta-test makes sure that the testing environment is properly reporting
// errors.

#include <svtkm/Assert.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestFail()
{
  SVTKM_TEST_FAIL("I expect this error.");
}

void BadTestAssert()
{
  SVTKM_TEST_ASSERT(0 == 1, "I expect this error.");
}

void GoodAssert()
{
  SVTKM_TEST_ASSERT(1 == 1, "Always true.");
  SVTKM_ASSERT(1 == 1);
}

} // anonymous namespace

int UnitTestContTesting(int argc, char* argv[])
{
  std::cout << "-------\nThis call should fail." << std::endl;
  if (svtkm::cont::testing::Testing::Run(TestFail, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "-------\nThis call should fail." << std::endl;
  if (svtkm::cont::testing::Testing::Run(BadTestAssert, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  std::cout << "-------\nThis call should pass." << std::endl;
  // This is what your main function typically looks like.
  return svtkm::cont::testing::Testing::Run(GoodAssert, argc, argv);
}
