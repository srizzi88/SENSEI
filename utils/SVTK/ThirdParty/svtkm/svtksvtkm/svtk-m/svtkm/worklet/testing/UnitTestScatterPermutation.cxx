//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/ScatterPermutation.h>

#include <svtkm/cont/ArrayHandle.h>
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
  using ControlSignature = void(CellSetIn cellset,
                                FieldOutPoint outPointId,
                                FieldOutPoint outVisit);
  using ExecutionSignature = void(InputIndex, VisitIndex, _2, _3);
  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterPermutation<>;

  SVTKM_CONT
  static ScatterType MakeScatter(const svtkm::cont::ArrayHandle<svtkm::Id>& permutation)
  {
    return ScatterType(permutation);
  }

  SVTKM_EXEC void operator()(svtkm::Id pointId,
                            svtkm::IdComponent visit,
                            svtkm::Id& outPointId,
                            svtkm::IdComponent& outVisit) const
  {
    outPointId = pointId;
    outVisit = visit;
  }
};

template <typename CellSetType>
void RunTest(const CellSetType& cellset, const svtkm::cont::ArrayHandle<svtkm::Id>& permutation)
{
  svtkm::cont::ArrayHandle<svtkm::Id> outPointId;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> outVisit;

  svtkm::worklet::DispatcherMapTopology<Worklet> dispatcher(Worklet::MakeScatter(permutation));
  dispatcher.Invoke(cellset, outPointId, outVisit);

  for (svtkm::Id i = 0; i < permutation.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(outPointId.GetPortalConstControl().Get(i) ==
                       permutation.GetPortalConstControl().Get(i),
                     "output point ids do not match the permutation");
    SVTKM_TEST_ASSERT(outVisit.GetPortalConstControl().Get(i) == 0, "incorrect visit index");
  }
}

void TestScatterPermutation()
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
  std::cout << "Testing with random permutations " << iterations << " times\n";
  std::cout << "Seed: " << seed << std::endl;
  for (int i = 1; i <= iterations; ++i)
  {
    std::cout << "iteration: " << i << "\n";

    svtkm::Id count = countDistribution(generator);
    svtkm::cont::ArrayHandle<svtkm::Id> permutation;
    permutation.Allocate(count);

    auto portal = permutation.GetPortalControl();
    std::cout << "using permutation:";
    for (svtkm::Id j = 0; j < count; ++j)
    {
      auto val = ptidDistribution(generator);
      std::cout << " " << val;
      portal.Set(j, val);
    }
    std::cout << "\n";

    RunTest(cellset, permutation);
  }
}

} // anonymous namespace

int UnitTestScatterPermutation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestScatterPermutation, argc, argv);
}
