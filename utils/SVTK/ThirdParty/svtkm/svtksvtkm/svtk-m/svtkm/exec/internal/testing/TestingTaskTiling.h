//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/StaticAssert.h>

#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/testing/Testing.h>

#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/arg/BasicArg.h>
#include <svtkm/exec/arg/Fetch.h>
#include <svtkm/exec/arg/ThreadIndicesBasic.h>

#include <svtkm/internal/FunctionInterface.h>
#include <svtkm/internal/Invocation.h>

#include <algorithm>
#include <vector>

namespace svtkm
{
namespace exec
{
namespace internal
{
namespace testing
{

struct TestExecObject
{
  SVTKM_EXEC_CONT
  TestExecObject()
    : Values(nullptr)
  {
  }

  SVTKM_EXEC_CONT
  TestExecObject(std::vector<svtkm::Id>& values)
    : Values(&values[0])
  {
  }

  SVTKM_EXEC_CONT
  TestExecObject(const TestExecObject& other) { Values = other.Values; }

  svtkm::Id* Values;
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
}
}
}
}

namespace svtkm
{
namespace exec
{
namespace arg
{

using namespace svtkm::exec::internal::testing;

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
    return execObject.Values[indices.GetInputIndex()] + 10 * indices.GetInputIndex();
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
    execObject.Values[indices.GetOutputIndex()] = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // svtkm::exec::arg

namespace svtkm
{
namespace exec
{
namespace internal
{
namespace testing
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

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void operator()(svtkm::Id input, svtkm::Id& output) const { output = input + 100; }

  SVTKM_EXEC
  svtkm::Id operator()(svtkm::Id input) const { return input + 200; }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const svtkm::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const svtkm::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const svtkm::Id flatThreadIndex = svtkm::Dot(threadIndex, svtkm::Id3(1, 8, 64));
    const svtkm::Id outIndex = threadToOut.Get(flatThreadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(flatThreadIndex,
                                               outToIn.Get(outIndex),
                                               visit.Get(outIndex),
                                               outIndex,
                                               globalThreadIndexOffset);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void operator()(svtkm::Id, svtkm::Id) const { this->RaiseError(ERROR_MESSAGE); }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const svtkm::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType,
            typename G>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const svtkm::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOutput,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const svtkm::Id index = svtkm::Dot(threadIndex, svtkm::Id3(1, 8, 64));
    const svtkm::Id outputIndex = threadToOutput.Get(index);
    return svtkm::exec::arg::ThreadIndicesBasic(index,
                                               outToIn.Get(outputIndex),
                                               visit.Get(outputIndex),
                                               outputIndex,
                                               globalThreadIndexOffset);
  }
};

template <typename DeviceAdapter>
void Test1DNormalTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D." << std::endl;

  std::vector<svtkm::Id> inputTestValues(100, 5);
  std::vector<svtkm::Id> outputTestValues(100, static_cast<svtkm::Id>(0xDEADDEAD));
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, svtkm::Id());

  svtkm::exec::internal::ErrorMessageBuffer errorMessage(nullptr, 0);
  task1.SetErrorMessageBuffer(errorMessage);

  task1(0, 90);
  task1(90, 99);
  task1(99, 100); //verify single value ranges work

  for (std::size_t i = 0; i < 100; ++i)
  {
    SVTKM_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    SVTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 100 + (30 * static_cast<svtkm::Id>(i)),
                     "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(outputTestValues.begin(), outputTestValues.end(), static_cast<svtkm::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, svtkm::Id());

  task2.SetErrorMessageBuffer(errorMessage);

  task2(0, 0); //verify zero value ranges work
  task2(0, 90);
  task2(90, 100);

  task2(0, 100); //verify that you can invoke worklets multiple times

  for (std::size_t i = 0; i < 100; ++i)
  {
    SVTKM_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    SVTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 200 + (30 * static_cast<svtkm::Id>(i)),
                     "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test1DErrorTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D with an error raised in the worklet." << std::endl;

  std::vector<svtkm::Id> inputTestValues(100, 5);
  std::vector<svtkm::Id> outputTestValues(100, static_cast<svtkm::Id>(0xDEADDEAD));

  TestExecObject arg1(inputTestValues);
  TestExecObject arg2(outputTestValues);

  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(arg1, arg2);

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task = TaskTypes::MakeTask(worklet, invocation, svtkm::Id());

  char message[1024];
  message[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task.SetErrorMessageBuffer(errorMessage);

  task(0, 100);

  SVTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  SVTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void Test3DNormalTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D." << std::endl;

  std::vector<svtkm::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<svtkm::Id> outputTestValues((8 * 8 * 8), static_cast<svtkm::Id>(0xDEADDEAD));
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;

  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, svtkm::Id3());
  for (svtkm::Id k = 0; k < 8; ++k)
  {
    for (svtkm::Id j = 0; j < 8; j += 2)
    {
      //verify that order is not required
      task1(0, 8, j + 1, k);
      task1(0, 8, j, k);
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    SVTKM_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    SVTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 100 + (30 * static_cast<svtkm::Id>(i)),
                     "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(outputTestValues.begin(), outputTestValues.end(), static_cast<svtkm::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);
  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, svtkm::Id3());

  //verify that linear order of values being processed is not presumed
  for (svtkm::Id i = 0; i < 8; ++i)
  {
    for (svtkm::Id j = 0; j < 8; ++j)
    {
      for (svtkm::Id k = 0; k < 8; ++k)
      {
        task2(i, i + 1, j, k);
      }
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    SVTKM_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    SVTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 200 + (30 * static_cast<svtkm::Id>(i)),
                     "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test3DErrorTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D with an error raised in the worklet." << std::endl;

  std::vector<svtkm::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<svtkm::Id> outputTestValues((8 * 8 * 8), static_cast<svtkm::Id>(0xDEADDEAD));
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename svtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation, svtkm::Id3());

  char message[1024];
  message[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task1.SetErrorMessageBuffer(errorMessage);

  for (svtkm::Id k = 0; k < 8; ++k)
  {
    for (svtkm::Id j = 0; j < 8; ++j)
    {
      task1(0, 8, j, k);
    }
  }

  SVTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  SVTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskTiling()
{
  Test1DNormalTaskTilingInvoke<DeviceAdapter>();
  Test1DErrorTaskTilingInvoke<DeviceAdapter>();

  Test3DNormalTaskTilingInvoke<DeviceAdapter>();
  Test3DErrorTaskTilingInvoke<DeviceAdapter>();
}
}
}
}
}
