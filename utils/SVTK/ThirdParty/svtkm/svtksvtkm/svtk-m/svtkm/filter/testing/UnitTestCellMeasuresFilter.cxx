//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/CellMeasures.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

template <typename IntegrationType>
void TestCellMeasuresFilter(svtkm::cont::DataSet& dataset,
                            const char* msg,
                            const std::vector<svtkm::Float32>& expected,
                            const IntegrationType&)
{
  std::cout << "Testing CellMeasures Filter on " << msg << "\n";

  svtkm::filter::CellMeasures<IntegrationType> vols;
  svtkm::cont::DataSet outputData = vols.Execute(dataset);

  SVTKM_TEST_ASSERT(vols.GetCellMeasureName().empty(), "Default output field name should be empty.");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfCells() == static_cast<svtkm::Id>(expected.size()),
                   "Wrong number of cells in the output dataset");

  // Check that the empty measure name above produced a field with the expected name.
  vols.SetCellMeasureName("measure");
  auto temp = outputData.GetField(vols.GetCellMeasureName()).GetData();
  SVTKM_TEST_ASSERT(temp.GetNumberOfValues() == static_cast<svtkm::Id>(expected.size()),
                   "Output field could not be found or was improper.");

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);
  SVTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == static_cast<svtkm::Id>(expected.size()),
                   "Wrong number of entries in the output dataset");

  for (unsigned int i = 0; i < static_cast<unsigned int>(expected.size()); ++i)
  {
    SVTKM_TEST_ASSERT(
      test_equal(resultArrayHandle.GetPortalConstControl().Get(svtkm::Id(i)), expected[i]),
      "Wrong result for CellMeasure filter");
  }
}

void TestCellMeasures()
{
  using svtkm::Volume;
  using svtkm::AllMeasures;

  svtkm::cont::testing::MakeTestDataSet factory;
  svtkm::cont::DataSet data;

  data = factory.Make3DExplicitDataSet2();
  TestCellMeasuresFilter(data, "explicit dataset 2", { -1.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet3();
  TestCellMeasuresFilter(data, "explicit dataset 3", { -1.f / 6.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet4();
  TestCellMeasuresFilter(data, "explicit dataset 4", { -1.f, -1.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet5();
  TestCellMeasuresFilter(
    data, "explicit dataset 5", { 1.f, 1.f / 3.f, 1.f / 6.f, -1.f / 2.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet6();
  TestCellMeasuresFilter(data,
                         "explicit dataset 6 (only volume)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.083426f, 0.25028f },
                         Volume());
  TestCellMeasuresFilter(
    data,
    "explicit dataset 6 (all)",
    { 0.999924f, 0.999924f, 0.f, 0.f, 3.85516f, 1.00119f, 0.083426f, 0.25028f },
    AllMeasures());
}

} // anonymous namespace

int UnitTestCellMeasuresFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellMeasures, argc, argv);
}
