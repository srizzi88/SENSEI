//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/internal/TaskSingular.h>

#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/arg/BasicArg.h>
#include <svtkm/exec/arg/ThreadIndicesBasic.h>

#include <svtkm/StaticAssert.h>

#include <svtkm/internal/FunctionInterface.h>
#include <svtkm/internal/Invocation.h>

#include <svtkm/testing/Testing.h>
namespace
{

struct TestExecObject
{
  SVTKM_EXEC_CONT
  TestExecObject()
    : Value(nullptr)
  {
  }

  SVTKM_EXEC_CONT
  TestExecObject(svtkm::Id* value)
    : Value(value)
  {
  }

  svtkm::Id* Value;
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
    return *execObject.Value + 10 * indices.GetInputIndex();
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
    *execObject.Value = value + 20 * indices.GetOutputIndex();
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
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
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
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
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

void TestNormalFunctorInvoke()
{
  std::cout << "Testing normal worklet invoke." << std::endl;

  svtkm::Id inputTestValue;
  svtkm::Id outputTestValue;
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                 TestExecObject(&outputTestValue));

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);
  using TaskSingular1 = svtkm::exec::internal::TaskSingular<TestWorkletProxy, InvocationType1>;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);
  TaskSingular1 taskInvokeWorklet1(worklet, invocation1);

  taskInvokeWorklet1(1);
  SVTKM_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  SVTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30, "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);
  using TaskSingular2 = svtkm::exec::internal::TaskSingular<TestWorkletProxy, InvocationType2>;
  InvocationType2 invocation2(execObjects);
  TaskSingular2 taskInvokeWorklet2(worklet, invocation2);

  taskInvokeWorklet2(2);
  SVTKM_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  SVTKM_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30 * 2, "Output value not set right.");
}

void TestErrorFunctorInvoke()
{
  std::cout << "Testing invoke with an error raised in the worklet." << std::endl;

  svtkm::Id inputTestValue = 5;
  svtkm::Id outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                 TestExecObject(&outputTestValue));

  using TaskSingular1 = svtkm::exec::internal::TaskSingular<TestWorkletErrorProxy, InvocationType1>;
  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);
  TaskSingular1 taskInvokeWorklet1 = TaskSingular1(worklet, invocation);

  char message[1024];
  message[0] = '\0';
  svtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  taskInvokeWorklet1.SetErrorMessageBuffer(errorMessage);
  taskInvokeWorklet1(1);

  SVTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  SVTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

void TestTaskSingular()
{
  TestNormalFunctorInvoke();
  TestErrorFunctorInvoke();
}

} // anonymous namespace

int UnitTestTaskSingular(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestTaskSingular, argc, argv);
}
