//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/Probe.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/worklet/CellDeepCopy.h>

namespace
{

svtkm::cont::DataSet MakeInputDataSet()
{
  auto input = svtkm::cont::DataSetBuilderUniform::Create(
    svtkm::Id2(4, 4), svtkm::make_Vec(0.0f, 0.0f), svtkm::make_Vec(1.0f, 1.0f));
  svtkm::cont::DataSetFieldAdd::AddPointField(
    input, "pointdata", svtkm::cont::make_ArrayHandleCounting(0.0f, 0.3f, 16));
  svtkm::cont::DataSetFieldAdd::AddCellField(
    input, "celldata", svtkm::cont::make_ArrayHandleCounting(0.0f, 0.7f, 9));
  return input;
}

svtkm::cont::DataSet MakeGeometryDataSet()
{
  auto geometry = svtkm::cont::DataSetBuilderUniform::Create(
    svtkm::Id2(9, 9), svtkm::make_Vec(0.7f, 0.7f), svtkm::make_Vec(0.35f, 0.35f));
  return geometry;
}

svtkm::cont::DataSet ConvertDataSetUniformToExplicit(const svtkm::cont::DataSet& uds)
{
  svtkm::cont::DataSet eds;

  svtkm::cont::CellSetExplicit<> cs;
  svtkm::worklet::CellDeepCopy::Run(uds.GetCellSet(), cs);
  eds.SetCellSet(cs);

  svtkm::cont::ArrayHandle<svtkm::Vec3f> points;
  svtkm::cont::ArrayCopy(uds.GetCoordinateSystem().GetData(), points);
  eds.AddCoordinateSystem(
    svtkm::cont::CoordinateSystem(uds.GetCoordinateSystem().GetName(), points));

  for (svtkm::IdComponent i = 0; i < uds.GetNumberOfFields(); ++i)
  {
    eds.AddField(uds.GetField(i));
  }

  return eds;
}

const std::vector<svtkm::Float32>& GetExpectedPointData()
{
  static std::vector<svtkm::Float32> expected = {
    1.05f,  1.155f, 1.26f,  1.365f, 1.47f,  1.575f, 1.68f,  0.0f,   0.0f,   1.47f,  1.575f, 1.68f,
    1.785f, 1.89f,  1.995f, 2.1f,   0.0f,   0.0f,   1.89f,  1.995f, 2.1f,   2.205f, 2.31f,  2.415f,
    2.52f,  0.0f,   0.0f,   2.31f,  2.415f, 2.52f,  2.625f, 2.73f,  2.835f, 2.94f,  0.0f,   0.0f,
    2.73f,  2.835f, 2.94f,  3.045f, 3.15f,  3.255f, 3.36f,  0.0f,   0.0f,   3.15f,  3.255f, 3.36f,
    3.465f, 3.57f,  3.675f, 3.78f,  0.0f,   0.0f,   3.57f,  3.675f, 3.78f,  3.885f, 3.99f,  4.095f,
    4.2f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,
    0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f
  };
  return expected;
}

const std::vector<svtkm::Float32>& GetExpectedCellData()
{
  static std::vector<svtkm::Float32> expected = {
    0.0f, 0.7f, 0.7f, 0.7f, 1.4f, 1.4f, 1.4f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f,
    3.5f, 3.5f, 0.0f, 0.0f, 2.1f, 2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 2.1f,
    2.8f, 2.8f, 2.8f, 3.5f, 3.5f, 3.5f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f,
    5.6f, 0.0f, 0.0f, 4.2f, 4.9f, 4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 4.2f, 4.9f,
    4.9f, 4.9f, 5.6f, 5.6f, 5.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
  return expected;
}

const std::vector<svtkm::UInt8>& GetExpectedHiddenPoints()
{
  static std::vector<svtkm::UInt8> expected = { 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2,
                                               2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0,
                                               2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0,
                                               0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2,
                                               2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  return expected;
}

const std::vector<svtkm::UInt8>& GetExpectedHiddenCells()
{
  static std::vector<svtkm::UInt8> expected = { 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2,
                                               2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
  return expected;
}

template <typename T>
void TestResultArray(const svtkm::cont::ArrayHandle<T>& result, const std::vector<T>& expected)
{
  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<svtkm::Id>(expected.size()),
                   "Incorrect field size");

  auto portal = result.GetPortalConstControl();
  svtkm::Id size = portal.GetNumberOfValues();
  for (svtkm::Id i = 0; i < size; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]),
                     "Incorrect field value");
  }
}

class TestProbe
{
private:
  using FieldArrayType = svtkm::cont::ArrayHandleCounting<svtkm::Float32>;

  static void ExplicitToUnifrom()
  {
    std::cout << "Testing Probe Explicit to Uniform:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = MakeGeometryDataSet();

    svtkm::worklet::Probe probe;
    probe.Run(input.GetCellSet(), input.GetCoordinateSystem(), geometry.GetCoordinateSystem());

    auto pf = probe.ProcessPointField(input.GetField("pointdata").GetData().Cast<FieldArrayType>());
    auto cf = probe.ProcessCellField(input.GetField("celldata").GetData().Cast<FieldArrayType>());
    auto hp = probe.GetHiddenPointsField();
    auto hc = probe.GetHiddenCellsField(geometry.GetCellSet());

    TestResultArray(pf, GetExpectedPointData());
    TestResultArray(cf, GetExpectedCellData());
    TestResultArray(hp, GetExpectedHiddenPoints());
    TestResultArray(hc, GetExpectedHiddenCells());
  }

  static void UniformToExplict()
  {
    std::cout << "Testing Probe Uniform to Explicit:\n";

    auto input = MakeInputDataSet();
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    svtkm::worklet::Probe probe;
    probe.Run(input.GetCellSet(), input.GetCoordinateSystem(), geometry.GetCoordinateSystem());

    auto pf = probe.ProcessPointField(input.GetField("pointdata").GetData().Cast<FieldArrayType>());
    auto cf = probe.ProcessCellField(input.GetField("celldata").GetData().Cast<FieldArrayType>());

    auto hp = probe.GetHiddenPointsField();
    auto hc = probe.GetHiddenCellsField(geometry.GetCellSet());

    TestResultArray(pf, GetExpectedPointData());
    TestResultArray(cf, GetExpectedCellData());
    TestResultArray(hp, GetExpectedHiddenPoints());
    TestResultArray(hc, GetExpectedHiddenCells());
  }

  static void ExplicitToExplict()
  {
    std::cout << "Testing Probe Explicit to Explicit:\n";

    auto input = ConvertDataSetUniformToExplicit(MakeInputDataSet());
    auto geometry = ConvertDataSetUniformToExplicit(MakeGeometryDataSet());

    svtkm::worklet::Probe probe;
    probe.Run(input.GetCellSet(), input.GetCoordinateSystem(), geometry.GetCoordinateSystem());

    auto pf = probe.ProcessPointField(input.GetField("pointdata").GetData().Cast<FieldArrayType>());
    auto cf = probe.ProcessCellField(input.GetField("celldata").GetData().Cast<FieldArrayType>());

    auto hp = probe.GetHiddenPointsField();
    auto hc = probe.GetHiddenCellsField(geometry.GetCellSet());

    TestResultArray(pf, GetExpectedPointData());
    TestResultArray(cf, GetExpectedCellData());
    TestResultArray(hp, GetExpectedHiddenPoints());
    TestResultArray(hc, GetExpectedHiddenCells());
  }

public:
  static void Run()
  {
    ExplicitToUnifrom();
    UniformToExplict();
    ExplicitToExplict();
  }
};

} // anonymous namespace

int UnitTestProbe(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestProbe::Run, argc, argv);
}
