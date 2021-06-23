//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/VariantArrayHandle.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

class TestMapFieldWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldInOut);
  using ExecutionSignature = _3(_1, _2, _3, WorkIndex);

  template <typename T>
  SVTKM_EXEC T operator()(const T& in, T& out, T& inout, svtkm::Id workIndex) const
  {
    if (!test_equal(in, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = static_cast<T>(in - T(100));
    if (!test_equal(inout, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }

    // We return the new value of inout. Since _3 is both an arg and return,
    // this tests that the return value is set after updating the arg values.
    return static_cast<T>(inout - T(100));
  }

  template <typename T1, typename T2, typename T3>
  SVTKM_EXEC T3 operator()(const T1&, const T2&, const T3&, svtkm::Id) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
    return svtkm::TypeTraits<T3>::ZeroInitialization();
  }
};

namespace mapfield
{
static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoStaticTestWorklet
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

    svtkm::cont::ArrayHandle<T> inputHandle = svtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> outputHandle, outputHandleAsPtr;
    svtkm::cont::ArrayHandle<T> inoutHandle, inoutHandleAsPtr;

    svtkm::cont::ArrayCopy(inputHandle, inoutHandle);
    svtkm::cont::ArrayCopy(inputHandle, inoutHandleAsPtr);

    std::cout << "Create and run dispatchers." << std::endl;
    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    dispatcher.Invoke(&inputHandle, &outputHandleAsPtr, &inoutHandleAsPtr);

    std::cout << "Check results." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(inoutHandle.GetPortalConstControl());
    CheckPortal(outputHandleAsPtr.GetPortalConstControl());
    CheckPortal(inoutHandleAsPtr.GetPortalConstControl());

    std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
    inputHandle.Shrink(ARRAY_SIZE / 2);
    bool exceptionThrown = false;
    try
    {
      dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    }
    catch (svtkm::cont::ErrorBadValue& error)
    {
      std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
      exceptionThrown = true;
    }
    SVTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
  }
};

template <typename WorkletType>
struct DoVariantTestWorklet
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

    svtkm::cont::ArrayHandle<T> inputHandle = svtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> outputHandle;
    svtkm::cont::ArrayHandle<T> inoutHandle;


    std::cout << "Create and run dispatcher with variant arrays." << std::endl;
    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;

    svtkm::cont::VariantArrayHandle inputVariant(inputHandle);

    { //Verify we can pass by value
      svtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      svtkm::cont::VariantArrayHandle outputVariant(outputHandle);
      svtkm::cont::VariantArrayHandle inoutVariant(inoutHandle);
      dispatcher.Invoke(inputVariant.ResetTypes(svtkm::List<T>{}),
                        outputVariant.ResetTypes(svtkm::List<T>{}),
                        inoutVariant.ResetTypes(svtkm::List<T>{}));
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());
    }

    { //Verify we can pass by pointer
      svtkm::cont::VariantArrayHandle outputVariant(outputHandle);
      svtkm::cont::VariantArrayHandle inoutVariant(inoutHandle);

      svtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(&inputVariant, outputHandle, inoutHandle);
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());

      svtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(inputHandle, &outputVariant, inoutHandle);
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());

      svtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(inputHandle, outputHandle, &inoutVariant);
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());
    }
  }
};

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  SVTKM_CONT void operator()(T t) const
  {
    DoStaticTestWorklet<WorkletType> sw;
    sw(t);
    DoVariantTestWorklet<WorkletType> dw;
    dw(t);
  }
};

void TestWorkletMapField(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

  svtkm::testing::Testing::TryTypes(mapfield::DoTestWorklet<TestMapFieldWorklet>(),
                                   svtkm::TypeListCommon());
}

} // mapfield namespace

int UnitTestWorkletMapField(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(mapfield::TestWorkletMapField, argc, argv);
}
