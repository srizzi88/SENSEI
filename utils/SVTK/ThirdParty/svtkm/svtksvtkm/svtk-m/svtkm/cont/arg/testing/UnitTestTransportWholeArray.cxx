//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagAtomicArray.h>
#include <svtkm/cont/arg/TransportTagWholeArrayIn.h>
#include <svtkm/cont/arg/TransportTagWholeArrayInOut.h>
#include <svtkm/cont/arg/TransportTagWholeArrayOut.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/StorageBasic.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

#define OFFSET 10

template <typename PortalType>
struct TestOutKernel : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("Out whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template <typename PortalType>
struct TestInKernel : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename PortalType>
struct TestInOutKernel : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In/Out whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, this->Portal.Get(index) + ValueType(OFFSET));
  }
};

template <typename AtomicType>
struct TestAtomicKernel : public svtkm::exec::FunctorBase
{
  SVTKM_CONT
  TestAtomicKernel(const AtomicType& atomicArray)
    : AtomicArray(atomicArray)
  {
  }

  AtomicType AtomicArray;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using ValueType = typename AtomicType::ValueType;
    this->AtomicArray.Add(0, static_cast<ValueType>(index));
  }
};

template <typename Device>
struct TryWholeArrayType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = svtkm::cont::ArrayHandle<T>;

    using InTransportType = svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagWholeArrayIn,
                                                       ArrayHandleType,
                                                       Device>;
    using InOutTransportType =
      svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagWholeArrayInOut,
                                 ArrayHandleType,
                                 Device>;
    using OutTransportType = svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagWholeArrayOut,
                                                        ArrayHandleType,
                                                        Device>;

    ArrayHandleType array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Check Transport WholeArrayOut" << std::endl;
    TestOutKernel<typename OutTransportType::ExecObjectType> outKernel;
    outKernel.Portal = OutTransportType()(array, nullptr, -1, -1);

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(outKernel, ARRAY_SIZE);

    CheckPortal(array.GetPortalConstControl());

    std::cout << "Check Transport WholeArrayIn" << std::endl;
    TestInKernel<typename InTransportType::ExecObjectType> inKernel;
    inKernel.Portal = InTransportType()(array, nullptr, -1, -1);

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(inKernel, ARRAY_SIZE);

    std::cout << "Check Transport WholeArrayInOut" << std::endl;
    TestInOutKernel<typename InOutTransportType::ExecObjectType> inOutKernel;
    inOutKernel.Portal = InOutTransportType()(array, nullptr, -1, -1);

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(inOutKernel, ARRAY_SIZE);

    SVTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Array size wrong?");
    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T expectedValue = TestValue(index, T()) + T(OFFSET);
      T retrievedValue = array.GetPortalConstControl().Get(index);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "In/Out array not set correctly.");
    }
  }
};

template <typename Device>
struct TryAtomicArrayType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>;

    using TransportType =
      svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagAtomicArray, ArrayHandleType, Device>;

    ArrayHandleType array;
    array.Allocate(1);
    array.GetPortalControl().Set(0, 0);

    std::cout << "Check Transport AtomicArray" << std::endl;
    TestAtomicKernel<typename TransportType::ExecObjectType> kernel(
      TransportType()(array, nullptr, -1, -1));

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    T result = array.GetPortalConstControl().Get(0);
    SVTKM_TEST_ASSERT(result == ((ARRAY_SIZE - 1) * ARRAY_SIZE) / 2,
                     "Got wrong summation in atomic array.");
  }
};

template <typename Device>
void TryArrayOutTransport(Device)
{
  svtkm::testing::Testing::TryTypes(TryWholeArrayType<Device>(), svtkm::TypeListCommon());
  svtkm::testing::Testing::TryTypes(TryAtomicArrayType<Device>(), svtkm::cont::AtomicArrayTypeList());
}

void TestWholeArrayTransport()
{
  std::cout << "Trying WholeArray transport." << std::endl;
  TryArrayOutTransport(svtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportWholeArray(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWholeArrayTransport, argc, argv);
}
