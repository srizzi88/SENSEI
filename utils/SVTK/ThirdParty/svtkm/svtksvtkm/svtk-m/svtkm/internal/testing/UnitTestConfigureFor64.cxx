//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/internal/ConfigureFor64.h>

#include <svtkm/Types.h>

#include <svtkm/testing/Testing.h>

// Size of 64 bits.
#define EXPECTED_SIZE 8

#if defined(SVTKM_NO_64BIT_IDS)
#error svtkm::Id an unexpected size.
#endif

#if defined(SVTKM_NO_DOUBLE_PRECISION)
#error svtkm::FloatDefault an unexpected size.
#endif

namespace
{

void TestTypeSizes()
{
  SVTKM_TEST_ASSERT(sizeof(svtkm::Id) == EXPECTED_SIZE, "svtkm::Id an unexpected size.");
  SVTKM_TEST_ASSERT(sizeof(svtkm::FloatDefault) == EXPECTED_SIZE,
                   "svtkm::FloatDefault an unexpected size.");
}
}

int UnitTestConfigureFor64(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestTypeSizes, argc, argv);
}
