//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleStreaming.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherStreamingMapField.h>

#include <vector>

namespace svtkm
{
namespace worklet
{
class SineWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1, WorkIndex);

  template <typename T>
  SVTKM_EXEC T operator()(T x, svtkm::Id& index) const
  {
    return (static_cast<T>(index) + svtkm::Sin(x));
  }
};
}
}

// Utility method to print input, output, and reference arrays
template <class T1, class T2, class T3>
void compareArrays(T1& a1, T2& a2, T3& a3, char const* text)
{
  for (svtkm::Id i = 0; i < a1.GetNumberOfValues(); ++i)
  {
    std::cout << a1.GetPortalConstControl().Get(i) << " " << a2.GetPortalConstControl().Get(i)
              << " " << a3.GetPortalConstControl().Get(i) << std::endl;
    SVTKM_TEST_ASSERT(
      test_equal(a2.GetPortalConstControl().Get(i), a3.GetPortalConstControl().Get(i), 0.01f),
      text);
  }
}

void TestStreamingSine()
{
  // Test the streaming worklet
  std::cout << "Testing streaming worklet:" << std::endl;

  const svtkm::Id N = 25;
  const svtkm::Id NBlocks = 4;
  svtkm::cont::ArrayHandle<svtkm::Float32> input, output, reference, summation;
  std::vector<svtkm::Float32> data(N), test(N);
  svtkm::Float32 testSum = 0.0f;
  for (svtkm::UInt32 i = 0; i < N; i++)
  {
    data[i] = static_cast<svtkm::Float32>(i);
    test[i] = static_cast<svtkm::Float32>(i) + static_cast<svtkm::Float32>(svtkm::Sin(data[i]));
    testSum += test[i];
  }
  input = svtkm::cont::make_ArrayHandle(data);

  using DeviceAlgorithms = svtkm::cont::Algorithm;
  svtkm::worklet::SineWorklet sineWorklet;
  svtkm::worklet::DispatcherStreamingMapField<svtkm::worklet::SineWorklet> dispatcher(sineWorklet);
  dispatcher.SetNumberOfBlocks(NBlocks);
  dispatcher.Invoke(input, output);

  reference = svtkm::cont::make_ArrayHandle(test);
  compareArrays(input, output, reference, "Wrong result for streaming sine worklet");

  svtkm::Float32 referenceSum, streamSum;

  // Test the streaming exclusive scan
  std::cout << "Testing streaming exclusive scan: " << std::endl;
  referenceSum = DeviceAlgorithms::ScanExclusive(input, summation);
  streamSum = DeviceAlgorithms::StreamingScanExclusive(4, input, output);
  SVTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming exclusive scan");
  compareArrays(input, output, summation, "Wrong result for streaming exclusive scan");

  // Test the streaming exclusive scan with binary operator
  std::cout << "Testing streaming exnclusive scan with binary operator: " << std::endl;
  svtkm::Float32 initValue = 0.0;
  referenceSum = DeviceAlgorithms::ScanExclusive(input, summation, svtkm::Maximum(), initValue);
  streamSum =
    DeviceAlgorithms::StreamingScanExclusive(4, input, output, svtkm::Maximum(), initValue);
  SVTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming exclusive scan with binary operator");
  compareArrays(
    input, output, summation, "Wrong result for streaming exclusive scan with binary operator");

  // Test the streaming reduce
  std::cout << "Testing streaming reduce: " << std::endl;
  referenceSum = DeviceAlgorithms::Reduce(input, 0.0f);
  streamSum = DeviceAlgorithms::StreamingReduce(4, input, 0.0f);
  std::cout << "Result: " << streamSum << " " << referenceSum << std::endl;
  SVTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f), "Wrong sum for streaming reduce");

  // Test the streaming reduce with binary operator
  std::cout << "Testing streaming reduce with binary operator: " << std::endl;
  referenceSum = DeviceAlgorithms::Reduce(input, 0.0f, svtkm::Maximum());
  streamSum = DeviceAlgorithms::StreamingReduce(4, input, 0.0f, svtkm::Maximum());
  std::cout << "Result: " << streamSum << " " << referenceSum << std::endl;
  SVTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming reduce with binary operator");
}

int UnitTestStreamingSine(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStreamingSine, argc, argv);
}
