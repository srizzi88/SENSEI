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

class TestAtomicArrayWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(WorkIndex, _2);
  using InputDomain = _1;

  template <typename AtomicArrayType>
  SVTKM_EXEC void operator()(const svtkm::Id& index, const AtomicArrayType& atomicArray) const
  {
    using ValueType = typename AtomicArrayType::ValueType;
    atomicArray.Add(0, static_cast<ValueType>(index));
  }
};

namespace map_whole_array
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

struct DoTestAtomicArrayWorklet
{
  using WorkletType = TestAtomicArrayWorklet;

  // This just demonstrates that the WholeArray tags support dynamic arrays.
  SVTKM_CONT
  void CallWorklet(const svtkm::cont::VariantArrayHandle& inOutArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(svtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                      inOutArray.ResetTypes<svtkm::cont::AtomicArrayTypeList>());
  }

  template <typename T>
  SVTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inOutValue = 0;

    svtkm::cont::ArrayHandle<T> inOutHandle = svtkm::cont::make_ArrayHandle(&inOutValue, 1);

    this->CallWorklet(svtkm::cont::VariantArrayHandle(inOutHandle));

    std::cout << "Check result." << std::endl;
    T result = inOutHandle.GetPortalConstControl().Get(0);

    SVTKM_TEST_ASSERT(result == (ARRAY_SIZE * (ARRAY_SIZE - 1)) / 2,
                     "Got wrong summation in atomic array.");
  }
};

void TestWorkletMapFieldExecArgAtomic(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with AtomicWholeArray on device adapter: " << id.GetName()
            << std::endl;
  svtkm::testing::Testing::TryTypes(map_whole_array::DoTestAtomicArrayWorklet(),
                                   svtkm::cont::AtomicArrayTypeList());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArrayAtomic(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArgAtomic, argc, argv);
}
