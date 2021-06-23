//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagArrayOut.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelOut : public svtkm::exec::FunctorBase
{
  PortalType Portal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template <typename Device>
struct TryArrayOutType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = svtkm::cont::ArrayHandle<T>;
    ArrayHandleType handle;

    using PortalType = typename ArrayHandleType::template ExecutionTypes<Device>::Portal;

    svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagArrayOut, ArrayHandleType, Device>
      transport;

    TestKernelOut<PortalType> kernel;
    kernel.Portal =
      transport(handle, svtkm::cont::ArrayHandleIndex(ARRAY_SIZE), ARRAY_SIZE, ARRAY_SIZE);

    SVTKM_TEST_ASSERT(handle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayOut transport did not allocate array correctly.");

    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    CheckPortal(handle.GetPortalConstControl());
  }
};

template <typename Device>
void TryArrayOutTransport(Device)
{
  svtkm::testing::Testing::TryTypes(TryArrayOutType<Device>());
}

void TestArrayOutTransport()
{
  std::cout << "Trying ArrayOut transport with serial device." << std::endl;
  TryArrayOutTransport(svtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportArrayOut(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayOutTransport, argc, argv);
}
