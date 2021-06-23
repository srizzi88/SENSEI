//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/arg/TransportTagExecObject.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/ExecutionObjectBase.h>

#include <svtkm/cont/testing/Testing.h>

#define EXPECTED_NUMBER 42

namespace
{

struct NotAnExecutionObject
{
};
struct InvalidExecutionObject : svtkm::cont::ExecutionObjectBase
{
};

template <typename Device>
struct ExecutionObject
{
  svtkm::Int32 Number;
};

struct TestExecutionObject : public svtkm::cont::ExecutionObjectBase
{
  svtkm::Int32 Number;

  template <typename Device>
  SVTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Number = this->Number;
    return object;
  }
};

template <typename Device>
struct TestKernel : public svtkm::exec::FunctorBase
{
  ExecutionObject<Device> Object;

  SVTKM_EXEC
  void operator()(svtkm::Id) const
  {
    if (this->Object.Number != EXPECTED_NUMBER)
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
void TryExecObjectTransport(Device)
{
  TestExecutionObject contObject;
  contObject.Number = EXPECTED_NUMBER;

  svtkm::cont::arg::Transport<svtkm::cont::arg::TransportTagExecObject, TestExecutionObject, Device>
    transport;

  TestKernel<Device> kernel;
  kernel.Object = transport(contObject, nullptr, 1, 1);

  svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);
}

void TestExecObjectTransport()
{
  std::cout << "Checking ExecObject queries." << std::endl;
  SVTKM_TEST_ASSERT(!svtkm::cont::internal::IsExecutionObjectBase<NotAnExecutionObject>::value,
                   "Bad query");
  SVTKM_TEST_ASSERT(svtkm::cont::internal::IsExecutionObjectBase<InvalidExecutionObject>::value,
                   "Bad query");
  SVTKM_TEST_ASSERT(svtkm::cont::internal::IsExecutionObjectBase<TestExecutionObject>::value,
                   "Bad query");

  SVTKM_TEST_ASSERT(!svtkm::cont::internal::HasPrepareForExecution<NotAnExecutionObject>::value,
                   "Bad query");
  SVTKM_TEST_ASSERT(!svtkm::cont::internal::HasPrepareForExecution<InvalidExecutionObject>::value,
                   "Bad query");
  SVTKM_TEST_ASSERT(svtkm::cont::internal::HasPrepareForExecution<TestExecutionObject>::value,
                   "Bad query");

  std::cout << "Trying ExecObject transport with serial device." << std::endl;
  TryExecObjectTransport(svtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportExecObject(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestExecObjectTransport, argc, argv);
}
