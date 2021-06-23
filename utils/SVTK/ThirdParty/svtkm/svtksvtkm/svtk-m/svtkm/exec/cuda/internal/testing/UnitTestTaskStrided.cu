//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/testing/Testing.h>

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>

#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/arg/BasicArg.h>
#include <svtkm/exec/arg/ThreadIndicesBasic.h>
#include <svtkm/exec/cuda/internal/TaskStrided.h>

#include <svtkm/StaticAssert.h>

#include <svtkm/internal/FunctionInterface.h>
#include <svtkm/internal/Invocation.h>

#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
// Disable warning "declared but never referenced"
// This file produces several false-positive warnings
// Eg: TestExecObject::TestExecObject, MyOutputToInputMapPortal::Get,
//     TestWorkletProxy::operator()
#pragma push
#pragma diag_suppress 177
#endif

namespace
{

struct TestExecObject
{
  SVTKM_EXEC_CONT
  TestExecObject(svtkm::exec::cuda::internal::ArrayPortalFromThrust<svtkm::Id> portal)
    : Portal(portal)
  {
  }

  svtkm::exec::cuda::internal::ArrayPortalFromThrust<svtkm::Id> Portal;
};

struct MyOutputToInputMapPortal
{
  using ValueType = svtkm::Id;
  SVTKM_EXEC_CONT
  svtkm::Id Get(svtkm::Id index) const { return index; }
};

struct MyVisitArrayPortal
{
  using ValueType = svtkm::IdComponent;
  SVTKM_EXEC_CONT
  svtkm::IdComponent Get(svtkm::Id) const { return 1; }
};

struct MyThreadToOutputMapPortal
{
  using ValueType = svtkm::Id;
  SVTKM_EXEC_CONT
  svtkm::Id Get(svtkm::Id index) const { return index; }
};

struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

// Missing TransportTag, but we are not testing that so we can leave it out.
struct TestControlSignatureTagInput
{
  using FetchTag = TestFetchTagInput;
};
struct TestControlSignatureTagOutput
{
  using FetchTag = TestFetchTagOutput;
};

} // anonymous namespace

namespace svtkm
{
namespace exec
{
namespace arg
{

template <>
struct Fetch<TestFetchTagInput,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesBasic,
             TestExecObject>
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC
  ValueType Load(const svtkm::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return execObject.Portal.Get(indices.GetInputIndex()) + 10 * indices.GetInputIndex();
  }

  SVTKM_EXEC
  void Store(const svtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesBasic,
             TestExecObject>
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC
  ValueType Load(const svtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&) const
  {
    // No-op
    return ValueType();
  }

  SVTKM_EXEC
  void Store(const svtkm::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObject& execObject,
             ValueType value) const
  {
    execObject.Portal.Set(indices.GetOutputIndex(), value + 20 * indices.GetOutputIndex());
  }
};
}
}
} // svtkm::exec::arg

namespace
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);
using TestControlInterface = svtkm::internal::FunctionInterface<TestControlSignature>;

using TestExecutionSignature1 = void(svtkm::exec::arg::BasicArg<1>, svtkm::exec::arg::BasicArg<2>);
using TestExecutionInterface1 = svtkm::internal::FunctionInterface<TestExecutionSignature1>;

using TestExecutionSignature2 = svtkm::exec::arg::BasicArg<2>(svtkm::exec::arg::BasicArg<1>);
using TestExecutionInterface2 = svtkm::internal::FunctionInterface<TestExecutionSignature2>;

using ExecutionParameterInterface =
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)>;

using InvocationType1 = svtkm::internal::Invocation<ExecutionParameterInterface,
                                                   TestControlInterface,
                                                   TestExecutionInterface1,
                                                   1,
                                                   MyOutputToInputMapPortal,
                                                   MyVisitArrayPortal,
                                                   MyThreadToOutputMapPortal>;

using InvocationType2 = svtkm::internal::Invocation<ExecutionParameterInterface,
                                                   TestControlInterface,
                                                   TestExecutionInterface2,
                                                   1,
                                                   MyOutputToInputMapPortal,
                                                   MyVisitArrayPortal,
                                                   MyThreadToOutputMapPortal>;

template <typename TaskType>
static __global__ void ScheduleTaskStrided(TaskType task, svtkm::Id start, svtkm::Id end)
{

  const svtkm::Id index = blockIdx.x * blockDim.x + threadIdx.x;
  const svtkm::Id inc = blockDim.x * gridDim.x;
  if (index >= start && index < end)
  {
    task(index, end, inc);
  }
}

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void operator()(svtkm::Id input, svtkm::Id& output) const { output = input + 100; }

  SVTKM_EXEC
  svtkm::Id operator()(svtkm::Id input) const { return input + 200; }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void operator()(svtkm::Id, svtkm::Id) const { this->RaiseError(ERROR_MESSAGE); }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

// Check behavior of InvocationToFetch helper class.

SVTKM_STATIC_ASSERT(
  (std::is_same<svtkm::exec::internal::detail::
                  InvocationToFetch<svtkm::exec::arg::ThreadIndicesBasic, InvocationType1, 1>::type,
                svtkm::exec::arg::Fetch<TestFetchTagInput,
                                       svtkm::exec::arg::AspectTagDefault,
                                       svtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

SVTKM_STATIC_ASSERT(
  (std::is_same<svtkm::exec::internal::detail::
                  InvocationToFetch<svtkm::exec::arg::ThreadIndicesBasic, InvocationType1, 2>::type,
                svtkm::exec::arg::Fetch<TestFetchTagOutput,
                                       svtkm::exec::arg::AspectTagDefault,
                                       svtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

SVTKM_STATIC_ASSERT(
  (std::is_same<svtkm::exec::internal::detail::
                  InvocationToFetch<svtkm::exec::arg::ThreadIndicesBasic, InvocationType2, 0>::type,
                svtkm::exec::arg::Fetch<TestFetchTagOutput,
                                       svtkm::exec::arg::AspectTagDefault,
                                       svtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

template <typename DeviceAdapter>
void TestNormalFunctorInvoke()
{
  std::cout << "Testing normal worklet invoke." << std::endl;

  svtkm::Id inputTestValues[3] = { 5, 5, 6 };

  svtkm::cont::ArrayHandle<svtkm::Id> input = svtkm::cont::make_ArrayHandle(inputTestValues, 3);
  svtkm::cont::ArrayHandle<svtkm::Id> output;

  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
      TestExecObject(output.PrepareForOutput(3, DeviceAdapter())));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, svtkm::Id());

  ScheduleTaskStrided<decltype(task1)><<<32, 256>>>(task1, 1, 2);
  cudaDeviceSynchronize();
  input.SyncControlArray();
  output.SyncControlArray();

  SVTKM_TEST_ASSERT(inputTestValues[1] == 5, "Input value changed.");
  SVTKM_TEST_ASSERT(output.GetPortalConstControl().Get(1) == inputTestValues[1] + 100 + 30,
                   "Output value not set right.");

  std::cout << "  Try return value." << std::endl;

  execObjects = svtkm::internal::make_FunctionInterface<void>(
    TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
    TestExecObject(output.PrepareForOutput(3, DeviceAdapter())));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, svtkm::Id());

  ScheduleTaskStrided<decltype(task2)><<<32, 256>>>(task2, 2, 3);
  cudaDeviceSynchronize();
  input.SyncControlArray();
  output.SyncControlArray();

  SVTKM_TEST_ASSERT(inputTestValues[2] == 6, "Input value changed.");
  SVTKM_TEST_ASSERT(output.GetPortalConstControl().Get(2) == inputTestValues[2] + 200 + 30 * 2,
                   "Output value not set right.");
}

template <typename DeviceAdapter>
void TestErrorFunctorInvoke()
{
  std::cout << "Testing invoke with an error raised in the worklet." << std::endl;

  svtkm::Id inputTestValue = 5;
  svtkm::Id outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);

  svtkm::cont::ArrayHandle<svtkm::Id> input = svtkm::cont::make_ArrayHandle(&inputTestValue, 1);
  svtkm::cont::ArrayHandle<svtkm::Id> output = svtkm::cont::make_ArrayHandle(&outputTestValue, 1);

  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
      TestExecObject(output.PrepareForInPlace(DeviceAdapter())));

  using TaskStrided1 =
    svtkm::exec::cuda::internal::TaskStrided1D<TestWorkletErrorProxy, InvocationType1>;
  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  auto task = TaskTypes::MakeTask(worklet, invocation, svtkm::Id());

  auto errorArray = Algorithm::GetPinnedErrorArray();
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(errorArray.DevicePtr, errorArray.Size);
  task.SetErrorMessageBuffer(errorMessage);

  ScheduleTaskStrided<decltype(task)><<<32, 256>>>(task, 1, 2);
  cudaDeviceSynchronize();

  SVTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  SVTKM_TEST_ASSERT(errorArray.HostPtr == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskStrided()
{
  TestNormalFunctorInvoke<DeviceAdapter>();
  TestErrorFunctorInvoke<DeviceAdapter>();
}

} // anonymous namespace

int UnitTestTaskStrided(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestTaskStrided<svtkm::cont::DeviceAdapterTagCuda>, argc, argv);
}

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
#pragma pop
#endif

#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
