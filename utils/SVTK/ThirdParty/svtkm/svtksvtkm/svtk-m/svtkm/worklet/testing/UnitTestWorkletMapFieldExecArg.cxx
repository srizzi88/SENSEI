//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/cont/VariantArrayHandle.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

struct SimpleExecObject : svtkm::cont::ExecutionObjectBase
{
  template <typename Device>
  Device PrepareForExecution(Device) const
  {
    return Device();
  }
};

struct TestExecObjectWorklet
{
  template <typename T>
  class Worklet : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut, FieldOut, ExecObject);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, Device);

    template <typename InPortalType, typename OutPortalType, typename DeviceTag>
    SVTKM_EXEC void operator()(const svtkm::Id& index,
                              const InPortalType& execIn,
                              OutPortalType& execOut,
                              T& out,
                              DeviceTag,
                              DeviceTag) const
    {
      SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

      if (!test_equal(execIn.Get(index), TestValue(index, T()) + T(100)))
      {
        this->RaiseError("Got wrong input value.");
      }
      out = static_cast<T>(execIn.Get(index) - T(100));
      execOut.Set(index, out);
    }
  };
};

namespace map_exec_field
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  SVTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    svtkm::cont::ArrayHandleIndex counting(ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> inputHandle = svtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> outputHandle;
    svtkm::cont::ArrayHandle<T> outputFieldArray;
    outputHandle.Allocate(ARRAY_SIZE);

    std::cout << "Create and run dispatcher." << std::endl;
    svtkm::worklet::DispatcherMapField<typename WorkletType::template Worklet<T>> dispatcher;
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldArray, SimpleExecObject());

    std::cout << "Check result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());

    std::cout << "Repeat with dynamic arrays." << std::endl;
    // Clear out output arrays.
    outputFieldArray = svtkm::cont::ArrayHandle<T>();
    outputHandle = svtkm::cont::ArrayHandle<T>();
    outputHandle.Allocate(ARRAY_SIZE);

    svtkm::cont::VariantArrayHandleBase<svtkm::List<T>> outputFieldDynamic(outputFieldArray);
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldDynamic, SimpleExecObject());

    std::cout << "Check dynamic array result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(outputFieldArray.GetPortalConstControl());
  }
};

void TestWorkletMapFieldExecArg(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  svtkm::testing::Testing::TryTypes(map_exec_field::DoTestWorklet<TestExecObjectWorklet>(),
                                   svtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldExecArg(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(
    map_exec_field::TestWorkletMapFieldExecArg, argc, argv);
}
