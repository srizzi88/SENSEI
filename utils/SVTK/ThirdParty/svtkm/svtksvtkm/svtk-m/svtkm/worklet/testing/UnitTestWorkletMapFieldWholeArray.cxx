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
#include <svtkm/cont/VariantArrayHandle.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/Testing.h>

class TestWholeArrayWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, WholeArrayInOut, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InPortalType, typename InOutPortalType, typename OutPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& index,
                            const InPortalType& inPortal,
                            const InOutPortalType& inOutPortal,
                            const OutPortalType& outPortal) const
  {
    using inT = typename InPortalType::ValueType;
    if (!test_equal(inPortal.Get(index), TestValue(index, inT())))
    {
      this->RaiseError("Got wrong input value.");
    }

    using inOutT = typename InOutPortalType::ValueType;
    if (!test_equal(inOutPortal.Get(index), TestValue(index, inOutT()) + inOutT(100)))
    {
      this->RaiseError("Got wrong input/output value.");
    }
    inOutPortal.Set(index, TestValue(index, inOutT()));

    using outT = typename OutPortalType::ValueType;
    outPortal.Set(index, TestValue(index, outT()));
  }
};

namespace map_whole_array
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

struct DoTestWholeArrayWorklet
{
  using WorkletType = TestWholeArrayWorklet;

  template <typename T>
  SVTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inArray[ARRAY_SIZE];
    T inOutArray[ARRAY_SIZE];

    for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inArray[index] = TestValue(index, T());
      inOutArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    svtkm::cont::ArrayHandle<T> inHandle = svtkm::cont::make_ArrayHandle(inArray, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> inOutHandle = svtkm::cont::make_ArrayHandle(inOutArray, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<T> outHandle;
    // Output arrays must be preallocated.
    outHandle.Allocate(ARRAY_SIZE);

    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(svtkm::cont::VariantArrayHandle(inHandle).ResetTypes(svtkm::List<T>{}),
                      svtkm::cont::VariantArrayHandle(inOutHandle).ResetTypes(svtkm::List<T>{}),
                      svtkm::cont::VariantArrayHandle(outHandle).ResetTypes(svtkm::List<T>{}));

    std::cout << "Check result." << std::endl;
    CheckPortal(inOutHandle.GetPortalConstControl());
    CheckPortal(outHandle.GetPortalConstControl());
  }
};

void TestWorkletMapFieldExecArg(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  svtkm::testing::Testing::TryTypes(map_whole_array::DoTestWholeArrayWorklet(),
                                   svtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArray(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArg, argc, argv);
}
