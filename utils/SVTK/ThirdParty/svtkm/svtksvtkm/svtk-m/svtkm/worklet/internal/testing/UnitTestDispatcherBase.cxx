//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/internal/DispatcherBase.h>

#include <svtkm/worklet/internal/WorkletBase.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

struct TestExecObjectIn
{
  SVTKM_EXEC_CONT
  TestExecObjectIn()
    : Array(nullptr)
  {
  }

  SVTKM_EXEC_CONT
  TestExecObjectIn(const svtkm::Id* array)
    : Array(array)
  {
  }

  const svtkm::Id* Array;
};

struct TestExecObjectOut
{
  SVTKM_EXEC_CONT
  TestExecObjectOut()
    : Array(nullptr)
  {
  }

  SVTKM_EXEC_CONT
  TestExecObjectOut(svtkm::Id* array)
    : Array(array)
  {
  }

  svtkm::Id* Array;
};

template <typename Device>
struct ExecutionObject
{
  svtkm::Id Value;
};

struct TestExecObjectType : svtkm::cont::ExecutionObjectBase
{
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
  template <typename Device>
  SVTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Value = this->Value;
    return object;
  }
  svtkm::Id Value;
};

struct TestExecObjectTypeBad
{ //this will fail as it doesn't inherit from svtkm::cont::ExecutionObjectBase
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
};

struct TestTypeCheckTag
{
};
struct TestTransportTagIn
{
};
struct TestTransportTagOut
{
};
struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

} // anonymous namespace

namespace svtkm
{
namespace cont
{
namespace arg
{

template <>
struct TypeCheck<TestTypeCheckTag, std::vector<svtkm::Id>>
{
  static constexpr bool value = true;
};

template <typename Device>
struct Transport<TestTransportTagIn, std::vector<svtkm::Id>, Device>
{
  using ExecObjectType = TestExecObjectIn;

  SVTKM_CONT
  ExecObjectType operator()(const std::vector<svtkm::Id>& contData,
                            const std::vector<svtkm::Id>&,
                            svtkm::Id inputRange,
                            svtkm::Id outputRange) const
  {
    SVTKM_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    SVTKM_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    return ExecObjectType(contData.data());
  }
};

template <typename Device>
struct Transport<TestTransportTagOut, std::vector<svtkm::Id>, Device>
{
  using ExecObjectType = TestExecObjectOut;

  SVTKM_CONT
  ExecObjectType operator()(const std::vector<svtkm::Id>& contData,
                            const std::vector<svtkm::Id>&,
                            svtkm::Id inputRange,
                            svtkm::Id outputRange) const
  {
    SVTKM_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    SVTKM_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    auto ptr = const_cast<svtkm::Id*>(contData.data());
    return ExecObjectType(ptr);
  }
};
}
}
} // namespace svtkm::cont::arg

namespace svtkm
{
namespace cont
{
namespace internal
{

template <>
struct DynamicTransformTraits<TestExecObjectType>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};
template <>
struct DynamicTransformTraits<TestExecObjectTypeBad>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};
}
}
} // namespace svtkm::cont::internal

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
             TestExecObjectIn>
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC
  ValueType Load(const svtkm::exec::arg::ThreadIndicesBasic indices,
                 const TestExecObjectIn& execObject) const
  {
    return execObject.Array[indices.GetInputIndex()];
  }

  SVTKM_EXEC
  void Store(const svtkm::exec::arg::ThreadIndicesBasic, const TestExecObjectIn&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput,
             svtkm::exec::arg::AspectTagDefault,
             svtkm::exec::arg::ThreadIndicesBasic,
             TestExecObjectOut>
{
  using ValueType = svtkm::Id;

  SVTKM_EXEC
  ValueType Load(const svtkm::exec::arg::ThreadIndicesBasic&, const TestExecObjectOut&) const
  {
    // No-op
    return ValueType();
  }

  SVTKM_EXEC
  void Store(const svtkm::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObjectOut& execObject,
             ValueType value) const
  {
    execObject.Array[indices.GetOutputIndex()] = value;
  }
};
}
}
} // svtkm::exec::arg

namespace
{

static constexpr svtkm::Id EXPECTED_EXEC_OBJECT_VALUE = 123;

class TestWorkletBase : public svtkm::worklet::internal::WorkletBase
{
public:
  struct TestIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagIn;
    using FetchTag = TestFetchTagInput;
  };
  struct TestOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagOut;
    using FetchTag = TestFetchTagOutput;
  };
};

class TestWorklet : public TestWorkletBase
{
public:
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = _3(_1, _2, WorkIndex);

  template <typename ExecObjectType>
  SVTKM_EXEC svtkm::Id operator()(svtkm::Id value, ExecObjectType execObject, svtkm::Id index) const
  {
    SVTKM_TEST_ASSERT(value == TestValue(index, svtkm::Id()), "Got bad value in worklet.");
    SVTKM_TEST_ASSERT(execObject.Value == EXPECTED_EXEC_OBJECT_VALUE,
                     "Got bad exec object in worklet.");
    return TestValue(index, svtkm::Id()) + 1000;
  }
};

#define ERROR_MESSAGE "Expected worklet error."

class TestErrorWorklet : public TestWorkletBase
{
public:
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ExecObjectType>
  SVTKM_EXEC void operator()(svtkm::Id, ExecObjectType, svtkm::Id) const
  {
    this->RaiseError(ERROR_MESSAGE);
  }
};

template <typename WorkletType>
class TestDispatcher : public svtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                                      WorkletType,
                                                                      TestWorkletBase>
{
  using Superclass = svtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                             WorkletType,
                                                             TestWorkletBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  SVTKM_CONT
  TestDispatcher(const WorkletType& worklet = WorkletType(),
                 const ScatterType& scatter = ScatterType())
    : Superclass(worklet, scatter)
  {
  }

  SVTKM_CONT
  template <typename Invocation>
  void DoInvoke(Invocation&& invocation) const
  {
    std::cout << "In TestDispatcher::DoInvoke()" << std::endl;
    this->BasicInvoke(invocation, ARRAY_SIZE);
  }

private:
  WorkletType Worklet;
};

void TestBasicInvoke()
{
  std::cout << "Test basic invoke" << std::endl;
  std::cout << "  Set up data." << std::endl;
  std::vector<svtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<svtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  std::size_t i = 0;
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++, i++)
  {
    inputArray[i] = TestValue(index, svtkm::Id());
    outputArray[i] = static_cast<svtkm::Id>(0xDEADDEAD);
  }

  std::cout << "  Create and run dispatcher." << std::endl;
  TestDispatcher<TestWorklet> dispatcher;
  dispatcher.Invoke(inputArray, execObject, &outputArray);

  std::cout << "  Check output of invoke." << std::endl;
  i = 0;
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++, i++)
  {
    SVTKM_TEST_ASSERT(outputArray[i] == TestValue(index, svtkm::Id()) + 1000,
                     "Got bad value from testing.");
  }
}

void TestInvokeWithError()
{
  std::cout << "Test invoke with error raised" << std::endl;
  std::cout << "  Set up data." << std::endl;
  std::vector<svtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<svtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  std::size_t i = 0;
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++, ++i)
  {
    inputArray[i] = TestValue(index, svtkm::Id());
    outputArray[i] = static_cast<svtkm::Id>(0xDEADDEAD);
  }

  try
  {
    std::cout << "  Create and run dispatcher that raises error." << std::endl;
    TestDispatcher<TestErrorWorklet> dispatcher;
    dispatcher.Invoke(&inputArray, execObject, outputArray);
    SVTKM_TEST_FAIL("Exception not thrown.");
  }
  catch (svtkm::cont::ErrorExecution& error)
  {
    std::cout << "  Got expected exception." << std::endl;
    std::cout << "  Exception message: " << error.GetMessage() << std::endl;
    SVTKM_TEST_ASSERT(error.GetMessage() == ERROR_MESSAGE, "Got unexpected error message.");
  }
}

void TestInvokeWithBadDynamicType()
{
  std::cout << "Test invoke with bad type" << std::endl;

  std::vector<svtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<svtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectTypeBad execObject;
  TestDispatcher<TestWorklet> dispatcher;

  try
  {
    std::cout << "  Second argument bad." << std::endl;
    dispatcher.Invoke(inputArray, execObject, outputArray);
    SVTKM_TEST_FAIL("Dispatcher did not throw expected error.");
  }
  catch (svtkm::cont::ErrorBadType& error)
  {
    std::cout << "    Got expected exception." << std::endl;
    std::cout << "    " << error.GetMessage() << std::endl;
    SVTKM_TEST_ASSERT(error.GetMessage().find(" 2 ") != std::string::npos,
                     "Parameter index not named in error message.");
  }
}

void TestDispatcherBase()
{
  TestBasicInvoke();
  TestInvokeWithError();
  TestInvokeWithBadDynamicType();
}

} // anonymous namespace

int UnitTestDispatcherBase(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestDispatcherBase, argc, argv);
}
