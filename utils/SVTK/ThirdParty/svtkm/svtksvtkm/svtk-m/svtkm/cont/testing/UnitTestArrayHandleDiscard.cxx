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
#include <svtkm/cont/ArrayHandleDiscard.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>
#include <svtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/BinaryOperators.h>

#include <algorithm>

namespace UnitTestArrayHandleDiscardDetail
{

template <typename ValueType>
struct Test
{
  static constexpr svtkm::Id ARRAY_SIZE = 100;
  static constexpr svtkm::Id NUM_KEYS = 3;

  using DeviceTag = svtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;
  using Handle = svtkm::cont::ArrayHandle<ValueType>;
  using DiscardHandle = svtkm::cont::ArrayHandleDiscard<ValueType>;
  using OutputPortal = typename Handle::PortalControl;
  using ReduceOp = svtkm::Add;

  // Test discard arrays by using the ReduceByKey algorithm. Two regular arrays
  // handles are provided as inputs, but the keys_output array is a discard
  // array handle. The values_output array should still be populated correctly.
  void TestReduceByKey()
  {
    // The reduction operator:
    ReduceOp op;

    // Prepare inputs / reference data:
    ValueType keyData[ARRAY_SIZE];
    ValueType valueData[ARRAY_SIZE];
    ValueType refData[NUM_KEYS];
    std::fill(refData, refData + NUM_KEYS, ValueType(0));
    for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const svtkm::Id key = i % NUM_KEYS;
      keyData[i] = static_cast<ValueType>(key);
      valueData[i] = static_cast<ValueType>(i * 2);
      refData[key] = op(refData[key], valueData[i]);
    }

    // Prepare array handles:
    Handle keys = svtkm::cont::make_ArrayHandle(keyData, ARRAY_SIZE);
    Handle values = svtkm::cont::make_ArrayHandle(valueData, ARRAY_SIZE);
    DiscardHandle output_keys;
    Handle output_values;

    Algorithm::SortByKey(keys, values);
    Algorithm::ReduceByKey(keys, values, output_keys, output_values, op);

    OutputPortal outputs = output_values.GetPortalControl();

    SVTKM_TEST_ASSERT(outputs.GetNumberOfValues() == NUM_KEYS,
                     "Unexpected number of output values from ReduceByKey.");

    for (svtkm::Id i = 0; i < NUM_KEYS; ++i)
    {
      SVTKM_TEST_ASSERT(outputs.Get(i) == refData[i], "Unexpected output value after ReduceByKey.");
    }
  }

  void TestPrepareExceptions()
  {
    DiscardHandle handle;
    handle.Allocate(50);

    try
    {
      handle.PrepareForInput(DeviceTag());
    }
    catch (svtkm::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    try
    {
      handle.PrepareForInPlace(DeviceTag());
    }
    catch (svtkm::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    // Shouldn't fail:
    handle.PrepareForOutput(ARRAY_SIZE, DeviceTag());
  }

  void operator()()
  {
    TestReduceByKey();
    TestPrepareExceptions();
  }
};

void TestArrayHandleDiscard()
{
  Test<svtkm::UInt8>()();
  Test<svtkm::Int16>()();
  Test<svtkm::Int32>()();
  Test<svtkm::Int64>()();
  Test<svtkm::Float32>()();
  Test<svtkm::Float64>()();
}

} // end namespace UnitTestArrayHandleDiscardDetail

int UnitTestArrayHandleDiscard(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleDiscardDetail;
  return svtkm::cont::testing::Testing::Run(TestArrayHandleDiscard, argc, argv);
}
