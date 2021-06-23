//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageImplicit.h>

#include <svtkm/cont/internal/IteratorFromArrayPortal.h>

#include <svtkm/cont/testing/Testing.h>

#if defined(SVTKM_STORAGE)
#undef SVTKM_STORAGE
#endif

#define SVTKM_STORAGE SVTKM_STORAGE_ERROR

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestImplicitStorage
{
  using ValueType = T;
  ValueType Temp;

  SVTKM_EXEC_CONT
  TestImplicitStorage()
    : Temp(1)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id svtkmNotUsed(index)) const { return Temp; }
};

template <typename T>
struct TemplatedTests
{
  using StorageTagType = svtkm::cont::StorageTagImplicit<TestImplicitStorage<T>>;
  using StorageType = svtkm::cont::internal::Storage<T, StorageTagType>;

  using ValueType = typename StorageType::ValueType;
  using PortalType = typename StorageType::PortalType;

  void BasicAllocation()
  {
    StorageType arrayStorage;

    // The implicit portal defined for this test always returns ARRAY_SIZE for the
    // number of values. We should get that.
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Implicit Storage GetNumberOfValues returned wrong size.");

    // Make sure you can allocate and shrink to any value <= the reported portal size.
    arrayStorage.Allocate(ARRAY_SIZE / 2);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE / 2,
                     "Cannot re-Allocate array to half size.");

    arrayStorage.Allocate(0);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "Cannot re-Allocate array to zero.");

    arrayStorage.Allocate(ARRAY_SIZE);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Cannot re-Allocate array to original size.");

    arrayStorage.Shrink(ARRAY_SIZE / 2);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE / 2,
                     "Cannot Shrink array to half size.");

    arrayStorage.Shrink(0);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "Cannot Shrink array to zero.");

    arrayStorage.Shrink(ARRAY_SIZE);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Cannot Shrink array to original size.");

    //verify that calling ReleaseResources doesn't throw an exception
    arrayStorage.ReleaseResources();

    //verify that you can allocate after releasing resources.
    arrayStorage.Allocate(ARRAY_SIZE);
  }

  void BasicAccess()
  {
    TestImplicitStorage<T> portal;
    svtkm::cont::ArrayHandle<T, StorageTagType> implictHandle(portal);
    SVTKM_TEST_ASSERT(implictHandle.GetNumberOfValues() == ARRAY_SIZE, "handle has wrong size");
    SVTKM_TEST_ASSERT(implictHandle.GetPortalConstControl().Get(0) == T(1),
                     "portals first values should be 1");
  }

  void operator()()
  {
    BasicAllocation();
    BasicAccess();
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

void TestStorageBasic()
{
  svtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestStorageImplicit(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStorageBasic, argc, argv);
}
