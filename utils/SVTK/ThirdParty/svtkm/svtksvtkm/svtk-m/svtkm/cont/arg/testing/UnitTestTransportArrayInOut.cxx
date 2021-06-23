//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagArrayInOut.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelInOut : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    ValueType inValue = this->Portal.Get(index);
    this->Portal.Set(index, inValue + inValue);
  }
};

template <typename Device>
struct TryArrayInOutType
{
  template <typename T>
  void operator()(T) const
  {
    T array[ARRAY_SIZE];
    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      array[index] = TestValue(index, T());
    }

    using ArrayHandleType = svtkm::cont::ArrayHandle<T>;
    ArrayHandleType handle = svtkm::cont::make_ArrayHandle(array, ARRAY_SIZE);

    using PortalType = typename ArrayHandleType::template ExecutionTypes<Device>::Portal;

    svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagArrayInOut, ArrayHandleType, Device>
      transport;

    TestKernelInOut<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE);

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    typename ArrayHandleType::PortalConstControl portal = handle.GetPortalConstControl();
    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE,
                     "Portal has wrong number of values.");
    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T expectedValue = TestValue(index, T()) + TestValue(index, T());
      T retrievedValue = portal.Get(index);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "Functor did not modify in place.");
    }
  }
};

template <typename Device>
void TryArrayInOutTransport(Device)
{
  svtkm::testing::Testing::TryTypes(TryArrayInOutType<Device>(), svtkm::TypeListCommon());
}

void TestArrayInOutTransport()
{
  std::cout << "Trying ArrayInOut transport with serial device." << std::endl;
  TryArrayInOutTransport(svtkm::cont::DeviceAdapterTagSerial());
}

} // anonymous namespace

int UnitTestTransportArrayInOut(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayInOutTransport, argc, argv);
}
