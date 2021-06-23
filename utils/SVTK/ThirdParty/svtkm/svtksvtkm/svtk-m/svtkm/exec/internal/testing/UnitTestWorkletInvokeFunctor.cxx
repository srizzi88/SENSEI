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

template <typename Invocation>
void CallDoWorkletInvokeFunctor(const Invocation& invocation, svtkm::Id index)
{
  const svtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(index);
  svtkm::exec::internal::detail::DoWorkletInvokeFunctor(
    TestWorkletProxy(),
    invocation,
    svtkm::exec::arg::ThreadIndicesBasic(index,
                                        invocation.OutputToInputMap.Get(outputIndex),
                                        invocation.VisitArray.Get(outputIndex),
                                        outputIndex));
}

void TestDoWorkletInvoke()
{
  std::cout << "Testing internal worklet invoke." << std::endl;

  svtkm::Id inputTestValue;
  svtkm::Id outputTestValue;
  svtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    svtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                 TestExecObject(&outputTestValue));

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(svtkm::internal::make_Invocation<1>(execObjects,
                                                                TestControlInterface(),
                                                                TestExecutionInterface1(),
                                                                MyOutputToInputMapPortal(),
                                                                MyVisitArrayPortal(),
                                                                MyThreadToOutputMapPortal()),
                             1);
  SVTKM_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  SVTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30, "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<svtkm::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(svtkm::internal::make_Invocation<1>(execObjects,
                                                                TestControlInterface(),
                                                                TestExecutionInterface2(),
                                                                MyOutputToInputMapPortal(),
                                                                MyVisitArrayPortal(),
                                                                MyThreadToOutputMapPortal()),
                             2);
  SVTKM_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  SVTKM_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30 * 2, "Output value not set right.");
}

void TestWorkletInvokeFunctor()
{
  TestDoWorkletInvoke();
}

} // anonymous namespace

int UnitTestWorkletInvokeFunctor(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestWorkletInvokeFunctor, argc, argv);
}
