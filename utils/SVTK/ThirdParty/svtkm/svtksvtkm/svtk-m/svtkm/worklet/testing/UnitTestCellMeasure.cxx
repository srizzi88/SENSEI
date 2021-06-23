//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/CellMeasure.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestCellMeasureUniform3D()
{
  std::cout << "Testing CellMeasure Worklet on 3D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> result;

  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellMeasure<svtkm::Volume>> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), result);

  svtkm::Float32 expected[4] = { 1.f, 1.f, 1.f, 1.f };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(svtkm::Id(i)), expected[i]),
                     "Wrong result for CellMeasure worklet on 3D uniform data");
  }
}

template <typename IntegrationType>
void TestCellMeasureWorklet(svtkm::cont::DataSet& dataset,
                            const char* msg,
                            const std::vector<svtkm::Float32>& expected,
                            const IntegrationType&)
{
  std::cout << "Testing CellMeasures Filter on " << msg << "\n";

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> result;

  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellMeasure<IntegrationType>> dispatcher;
  dispatcher.Invoke(dataset.GetCellSet(), dataset.GetCoordinateSystem(), result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<svtkm::Id>(expected.size()),
                   "Wrong number of values in the output array");

  for (unsigned int i = 0; i < static_cast<unsigned int>(expected.size()); ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(svtkm::Id(i)), expected[i]),
                     "Wrong result for CellMeasure filter");
  }
}

void TestCellMeasure()
{
  using svtkm::ArcLength;
  using svtkm::Area;
  using svtkm::Volume;
  using svtkm::AllMeasures;

  TestCellMeasureUniform3D();

  svtkm::cont::testing::MakeTestDataSet factory;
  svtkm::cont::DataSet data;

  data = factory.Make3DExplicitDataSet2();
  TestCellMeasureWorklet(data, "explicit dataset 2", { -1.f }, Volume());

  data = factory.Make3DExplicitDataSet3();
  TestCellMeasureWorklet(data, "explicit dataset 3", { -1.f / 6.f }, Volume());

  data = factory.Make3DExplicitDataSet4();
  TestCellMeasureWorklet(data, "explicit dataset 4", { -1.f, -1.f }, Volume());

  data = factory.Make3DExplicitDataSet5();
  TestCellMeasureWorklet(
    data, "explicit dataset 5", { 1.f, 1.f / 3.f, 1.f / 6.f, -1.f / 2.f }, Volume());

  data = factory.Make3DExplicitDataSet6();
  TestCellMeasureWorklet(
    data,
    "explicit dataset 6 (all)",
    { 0.999924f, 0.999924f, 0.f, 0.f, 3.85516f, 1.00119f, 0.083426f, 0.25028f },
    AllMeasures());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (arc length)",
                         { 0.999924f, 0.999924f, 0.f, 0.f, 0.0f, 0.0f, 0.0f, 0.0f },
                         ArcLength());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (area)",
                         { 0.0f, 0.0f, 0.f, 0.f, 3.85516f, 1.00119f, 0.0f, 0.0f },
                         Area());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (volume)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.083426f, 0.25028f },
                         Volume());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (empty)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.0f, 0.0f },
                         svtkm::List<>());
}
}

int UnitTestCellMeasure(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellMeasure, argc, argv);
}
