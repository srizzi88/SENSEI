//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagArrayIn.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelIn : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
struct TryArrayInType
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

    using PortalType = typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst;

    svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagArrayIn, ArrayHandleType, Device>
      transport;

    TestKernelIn<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE);

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
  }
};

template <typename Device>
void TryArrayInTransport(Device)
{
  svtkm::testing::Testing::TryTypes(TryArrayInType<Device>());
}

void TestArrayInTransport()
{
  std::cout << "Trying ArrayIn transport with serial device." << std::endl;
  TryArrayInTransport(svtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportArrayIn(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayInTransport, argc, argv);
}
