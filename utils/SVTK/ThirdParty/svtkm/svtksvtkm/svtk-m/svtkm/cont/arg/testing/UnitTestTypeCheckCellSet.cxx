//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TypeCheckTagCellSet.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetStructured.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

struct TestNotCellSet
{
};

void TestCheckCellSet()
{
  std::cout << "Checking reporting of type checking cell set." << std::endl;

  using svtkm::cont::arg::TypeCheck;
  using svtkm::cont::arg::TypeCheckTagCellSet;

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, svtkm::cont::CellSetExplicit<>>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, svtkm::cont::CellSetStructured<2>>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, svtkm::cont::CellSetStructured<3>>::value),
                   "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, TestNotCellSet>::value), "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, svtkm::Id>::value), "Type check failed.");

  SVTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, svtkm::cont::ArrayHandle<svtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckCellSet(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCheckCellSet, argc, argv);
}
