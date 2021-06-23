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
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleVirtual.h>

#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>
#include <svtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/BinaryOperators.h>

#include <algorithm>

namespace UnitTestArrayHandleVirtualDetail
{

template <typename ValueType>
struct Test
{
  static constexpr svtkm::Id ARRAY_SIZE = 100;
  static constexpr svtkm::Id NUM_KEYS = 3;

  using ArrayHandle = svtkm::cont::ArrayHandle<ValueType>;
  using VirtHandle = svtkm::cont::ArrayHandleVirtual<ValueType>;
  using DeviceTag = svtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

  void TestConstructors()
  {
    VirtHandle nullStorage;
    SVTKM_TEST_ASSERT(nullStorage.GetStorage().GetStorageVirtual() == nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");

    VirtHandle fromArrayHandle{ ArrayHandle{} };
    SVTKM_TEST_ASSERT(fromArrayHandle.GetStorage().GetStorageVirtual() != nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");
    SVTKM_TEST_ASSERT(svtkm::cont::IsType<ArrayHandle>(fromArrayHandle),
                     "ArrayHandleVirtual should contain a ArrayHandle<ValueType>.");

    VirtHandle fromVirtHandle(fromArrayHandle);
    SVTKM_TEST_ASSERT(fromVirtHandle.GetStorage().GetStorageVirtual() != nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");
    SVTKM_TEST_ASSERT(svtkm::cont::IsType<ArrayHandle>(fromVirtHandle),
                     "ArrayHandleVirtual should contain a ArrayHandle<ValueType>.");

    VirtHandle fromNullPtrHandle(nullStorage);
    SVTKM_TEST_ASSERT(fromNullPtrHandle.GetStorage().GetStorageVirtual() == nullptr,
                     "storage should be empty when constructing from a ArrayHandleVirtual that has "
                     "nullptr storage.");
    SVTKM_TEST_ASSERT((svtkm::cont::IsType<ArrayHandle>(fromNullPtrHandle) == false),
                     "ArrayHandleVirtual shouldn't match any type with nullptr storage.");
  }


  void TestMoveConstructors()
  {
    //test ArrayHandle move constructor
    {
      ArrayHandle handle;
      VirtHandle virt(std::move(handle));
      SVTKM_TEST_ASSERT(
        svtkm::cont::IsType<ArrayHandle>(virt),
        "ArrayHandleVirtual should be valid after move constructor ArrayHandle<ValueType>.");
    }

    //test ArrayHandleVirtual move constructor
    {
      ArrayHandle handle;
      VirtHandle virt(std::move(handle));
      VirtHandle virt2(std::move(virt));
      SVTKM_TEST_ASSERT(
        svtkm::cont::IsType<ArrayHandle>(virt2),
        "ArrayHandleVirtual should be valid after move constructor ArrayHandleVirtual<ValueType>.");
    }
  }

  void TestAssignmentOps()
  {
    //test assignment operator from ArrayHandleVirtual
    {
      VirtHandle virt;
      virt = VirtHandle{ ArrayHandle{} };
      SVTKM_TEST_ASSERT(svtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after assignment op from AHV.");
    }

    //test assignment operator from ArrayHandle
    {
      VirtHandle virt = svtkm::cont::ArrayHandleCounting<ValueType>{};
      virt = ArrayHandle{};
      SVTKM_TEST_ASSERT(svtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after assignment op from AH.");
    }

    //test move assignment operator from ArrayHandleVirtual
    {
      VirtHandle temp{ ArrayHandle{} };
      VirtHandle virt;
      virt = std::move(temp);
      SVTKM_TEST_ASSERT(svtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after move assignment op from AHV.");
    }

    //test move assignment operator from ArrayHandle
    {
      svtkm::cont::ArrayHandleCounting<ValueType> temp;
      VirtHandle virt;
      virt = std::move(temp);
      SVTKM_TEST_ASSERT(svtkm::cont::IsType<decltype(temp)>(virt),
                       "ArrayHandleVirtual should be valid after move assignment op from AH.");
    }
  }

  void TestPrepareForExecution()
  {
    svtkm::cont::ArrayHandle<ValueType> handle;
    handle.Allocate(50);

    VirtHandle virt(std::move(handle));

    try
    {
      virt.PrepareForInput(DeviceTag());
      virt.PrepareForInPlace(DeviceTag());
      virt.PrepareForOutput(ARRAY_SIZE, DeviceTag());
    }
    catch (svtkm::cont::ErrorBadValue&)
    {
      // un-expected failure.
      SVTKM_TEST_FAIL(
        "Unexpected error when using Prepare* on an ArrayHandleVirtual with StorageAny.");
    }
  }


  void TestIsType()
  {
    svtkm::cont::ArrayHandle<ValueType> handle;
    VirtHandle virt(std::move(handle));

    SVTKM_TEST_ASSERT(svtkm::cont::IsType<decltype(virt)>(virt),
                     "virt should by same type as decltype(virt)");
    SVTKM_TEST_ASSERT(svtkm::cont::IsType<decltype(handle)>(virt),
                     "virt should by same type as decltype(handle)");

    svtkm::cont::ArrayHandle<svtkm::Vec<ValueType, 3>> vecHandle;
    SVTKM_TEST_ASSERT(!svtkm::cont::IsType<decltype(vecHandle)>(virt),
                     "virt shouldn't by same type as decltype(vecHandle)");
  }

  void TestCast()
  {

    svtkm::cont::ArrayHandle<ValueType> handle;
    VirtHandle virt(handle);

    auto c1 = svtkm::cont::Cast<decltype(virt)>(virt);
    SVTKM_TEST_ASSERT(c1 == virt, "virt should cast to VirtHandle");

    auto c2 = svtkm::cont::Cast<decltype(handle)>(virt);
    SVTKM_TEST_ASSERT(c2 == handle, "virt should cast to HandleType");

    using VecHandle = svtkm::cont::ArrayHandle<svtkm::Vec<ValueType, 3>>;
    try
    {
      auto c3 = svtkm::cont::Cast<VecHandle>(virt);
      SVTKM_TEST_FAIL("Cast of T to Vec<T,3> should have failed");
    }
    catch (svtkm::cont::ErrorBadType&)
    {
    }
  }

  void operator()()
  {
    TestConstructors();
    TestMoveConstructors();
    TestAssignmentOps();
    TestPrepareForExecution();
    TestIsType();
    TestCast();
  }
};

void TestArrayHandleVirtual()
{
  Test<svtkm::UInt8>()();
  Test<svtkm::Int16>()();
  Test<svtkm::Int32>()();
  Test<svtkm::Int64>()();
  Test<svtkm::Float32>()();
  Test<svtkm::Float64>()();
}

} // end namespace UnitTestArrayHandleVirtualDetail

int UnitTestArrayHandleVirtual(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleVirtualDetail;
  return svtkm::cont::testing::Testing::Run(TestArrayHandleVirtual, argc, argv);
}
