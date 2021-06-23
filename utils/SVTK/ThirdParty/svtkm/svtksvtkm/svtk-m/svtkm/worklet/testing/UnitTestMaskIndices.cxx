//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/MaskIndices.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

class Worklet : public svtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInOutPoint outPointId);
  using ExecutionSignature = void(InputIndex, _2);
  using InputDomain = _1;

  using MaskType = svtkm::worklet::MaskIndices;

  SVTKM_EXEC void operator()(svtkm::Id pointId, svtkm::Id& outPointId) const { outPointId = pointId; }
};

template <typename CellSetType>
void RunTest(const CellSetType& cellset, const svtkm::cont::ArrayHandle<svtkm::Id>& indices)
{
  svtkm::Id numPoints = cellset.GetNumberOfPoints();
  svtkm::cont::ArrayHandle<svtkm::Id> outPointId;
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant<svtkm::Id>(-1, numPoints), outPointId);

  svtkm::worklet::DispatcherMapTopology<Worklet> dispatcher(svtkm::worklet::MaskIndices{ indices });
  dispatcher.Invoke(cellset, outPointId);

  svtkm::cont::ArrayHandle<svtkm::Int8> stencil;
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant<svtkm::Int8>(0, numPoints), stencil);

  // Check that output that should be written was.
  for (svtkm::Id i = 0; i < indices.GetNumberOfValues(); ++i)
  {
    // All unmasked indices should have been copied to the output.
    svtkm::Id unmaskedIndex = indices.GetPortalConstControl().Get(i);
    svtkm::Id writtenValue = outPointId.GetPortalConstControl().Get(unmaskedIndex);
    SVTKM_TEST_ASSERT(unmaskedIndex == writtenValue,
                     "Did not pass unmasked index. Expected ",
                     unmaskedIndex,
                     ". Got ",
                     writtenValue);

    // Mark index as passed.
    stencil.GetPortalControl().Set(unmaskedIndex, 1);
  }

  // Check that output that should not be written was not.
  for (svtkm::Id i = 0; i < numPoints; ++i)
  {
    if (stencil.GetPortalConstControl().Get(i) == 0)
    {
      svtkm::Id foundValue = outPointId.GetPortalConstControl().Get(i);
      SVTKM_TEST_ASSERT(foundValue == -1,
                       "Expected index ",
                       i,
                       " to be unwritten but was filled with ",
                       foundValue);
    }
  }
}

void TestMaskIndices()
{
  svtkm::cont::DataSet dataset = svtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  auto cellset = dataset.GetCellSet();
  svtkm::Id numberOfPoints = cellset.GetNumberOfPoints();

  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(std::time(nullptr));

  std::default_random_engine generator;
  generator.seed(seed);
  std::uniform_int_distribution<svtkm::Id> countDistribution(1, 2 * numberOfPoints);
  std::uniform_int_distribution<svtkm::Id> ptidDistribution(0, numberOfPoints - 1);

  const int iterations = 5;
  std::cout << "Testing with random indices " << iterations << " times\n";
  std::cout << "Seed: " << seed << std::endl;
  for (int i = 1; i <= iterations; ++i)
  {
    std::cout << "iteration: " << i << "\n";

    svtkm::Id count = countDistribution(generator);
    svtkm::cont::ArrayHandle<svtkm::Id> indices;
    indices.Allocate(count);

    // Note that it is possible that the same index will be written twice, which is generally
    // a bad idea with MaskIndices. However, the worklet will write the same value for each
    // instance, so we should still get the correct result.
    auto portal = indices.GetPortalControl();
    std::cout << "using indices:";
    for (svtkm::Id j = 0; j < count; ++j)
    {
      auto val = ptidDistribution(generator);
      std::cout << " " << val;
      portal.Set(j, val);
    }
    std::cout << "\n";

    RunTest(cellset, indices);
  }
}

} // anonymous namespace

int UnitTestMaskIndices(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestMaskIndices, argc, argv);
}
