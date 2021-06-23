//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageBasic.h>

#include <svtkm/VecTraits.h>
#include <svtkm/cont/testing/Testing.h>

#if defined(SVTKM_STORAGE)
#undef SVTKM_STORAGE
#endif

#define SVTKM_STORAGE SVTKM_STORAGE_ERROR

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TemplatedTests
{
  using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>;
  using ValueType = typename StorageType::ValueType;
  using PortalType = typename StorageType::PortalType;

  void SetStorage(StorageType& array, const ValueType& value)
  {
    PortalType portal = array.GetPortal();
    for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      portal.Set(index, value);
    }
  }

  bool CheckStorage(StorageType& array, const ValueType& value)
  {
    PortalType portal = array.GetPortal();
    for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      if (!test_equal(portal.Get(index), value))
      {
        return false;
      }
    }
    return true;
  }

  typename svtkm::VecTraits<ValueType>::ComponentType STOLEN_ARRAY_VALUE() { return 29; }

  /// Returned value should later be passed to StealArray2.  It is best to
  /// put as much between the two test parts to maximize the chance of a
  /// deallocated array being overridden (and thus detected).
  ValueType* StealArray1()
  {
    ValueType* stolenArray;

    ValueType stolenArrayValue = ValueType(STOLEN_ARRAY_VALUE());

    StorageType stealMyArray;
    stealMyArray.Allocate(ARRAY_SIZE);
    this->SetStorage(stealMyArray, stolenArrayValue);

    SVTKM_TEST_ASSERT(stealMyArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Array not properly allocated.");
    // This call steals the array and prevents deallocation.
    SVTKM_TEST_ASSERT(stealMyArray.WillDeallocate() == true,
                     "Array to be stolen needs to be owned by SVTK-m");
    auto stolen = stealMyArray.StealArray();
    stolenArray = stolen.first;
    SVTKM_TEST_ASSERT(stealMyArray.WillDeallocate() == false,
                     "Stolen array should not be owned by SVTK-m");

    return stolenArray;
  }
  void StealArray2(ValueType* stolenArray)
  {
    ValueType stolenArrayValue = ValueType(STOLEN_ARRAY_VALUE());

    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      SVTKM_TEST_ASSERT(test_equal(stolenArray[index], stolenArrayValue),
                       "Stolen array did not retain values.");
    }
    typename StorageType::AllocatorType allocator;
    allocator.deallocate(stolenArray);
  }

  void BasicAllocation()
  {
    StorageType arrayStorage;
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "New array storage not zero sized.");

    arrayStorage.Allocate(ARRAY_SIZE);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Array not properly allocated.");

    const ValueType BASIC_ALLOC_VALUE = ValueType(48);
    this->SetStorage(arrayStorage, BASIC_ALLOC_VALUE);
    SVTKM_TEST_ASSERT(this->CheckStorage(arrayStorage, BASIC_ALLOC_VALUE),
                     "Array not holding value.");

    arrayStorage.Allocate(ARRAY_SIZE * 2);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Array not reallocated correctly.");

    arrayStorage.Shrink(ARRAY_SIZE);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Array Shrnk failed to resize.");

    arrayStorage.ReleaseResources();
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "Array not released correctly.");

    try
    {
      arrayStorage.Shrink(ARRAY_SIZE);
      SVTKM_TEST_ASSERT(true == false,
                       "Array shrink do a larger size was possible. This can't be allowed.");
    }
    catch (svtkm::cont::ErrorBadValue&)
    {
    }
  }

  void UserFreeFunction()
  {
    ValueType* temp = new ValueType[ARRAY_SIZE];
    StorageType arrayStorage(
      temp, ARRAY_SIZE, [](void* ptr) { delete[] static_cast<ValueType*>(ptr); });
    SVTKM_TEST_ASSERT(temp == arrayStorage.GetArray(),
                     "improper pointer after telling storage to own user allocated memory");

    const ValueType BASIC_ALLOC_VALUE = ValueType(48);
    this->SetStorage(arrayStorage, BASIC_ALLOC_VALUE);
    SVTKM_TEST_ASSERT(this->CheckStorage(arrayStorage, BASIC_ALLOC_VALUE),
                     "Array not holding value.");

    arrayStorage.Allocate(ARRAY_SIZE * 2);
    SVTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Array not reallocated correctly.");
  }

  void operator()()
  {
    ValueType* stolenArray = StealArray1();

    BasicAllocation();
    UserFreeFunction();

    StealArray2(stolenArray);
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

int UnitTestStorageBasic(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStorageBasic, argc, argv);
}
