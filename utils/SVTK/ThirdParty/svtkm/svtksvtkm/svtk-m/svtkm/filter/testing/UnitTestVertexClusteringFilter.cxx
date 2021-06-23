//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/VertexClustering.h>

using svtkm::cont::testing::MakeTestDataSet;

namespace
{
}

void TestVertexClustering()
{
  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose();

  svtkm::filter::VertexClustering clustering;

  clustering.SetNumberOfDivisions(svtkm::Id3(3, 3, 3));
  clustering.SetFieldsToPass({ "pointvar", "cellvar" });
  svtkm::cont::DataSet output = clustering.Execute(dataSet);
  SVTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Number of output coordinate systems mismatch");

  using FieldArrayType = svtkm::cont::ArrayHandle<svtkm::Float32>;
  FieldArrayType pointvar = output.GetPointField("pointvar").GetData().Cast<FieldArrayType>();
  FieldArrayType cellvar = output.GetCellField("cellvar").GetData().Cast<FieldArrayType>();

  // test
  const svtkm::Id output_points = 7;
  svtkm::Float64 output_point[output_points][3] = {
    { 0.0174716, 0.0501928, 0.0930275 }, { 0.0307091, 0.1521420, 0.05392490 },
    { 0.0174172, 0.1371240, 0.1245530 }, { 0.0480879, 0.1518740, 0.10733400 },
    { 0.0180085, 0.2043600, 0.1453160 }, { -.000129414, 0.00247137, 0.17656100 },
    { 0.0108188, 0.1527740, 0.1679140 }
  };

  svtkm::Float32 output_pointvar[output_points] = { 28.f, 19.f, 25.f, 15.f, 16.f, 21.f, 30.f };
  svtkm::Float32 output_cellvar[] = { 145.f, 134.f, 138.f, 140.f, 149.f, 144.f };

  {
    using CellSetType = svtkm::cont::CellSetSingleType<>;
    CellSetType cellSet;
    output.GetCellSet().CopyTo(cellSet);
    auto cellArray =
      cellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
    std::cerr << "output_pointIds = " << cellArray.GetNumberOfValues() << "\n";
    std::cerr << "output_pointId[] = ";
    svtkm::cont::printSummary_ArrayHandle(cellArray, std::cerr, true);
  }

  {
    auto pointArray = output.GetCoordinateSystem(0).GetData();
    std::cerr << "output_points = " << pointArray.GetNumberOfValues() << "\n";
    std::cerr << "output_point[] = ";
    svtkm::cont::printSummary_ArrayHandle(pointArray, std::cerr, true);
  }

  svtkm::cont::printSummary_ArrayHandle(pointvar, std::cerr, true);
  svtkm::cont::printSummary_ArrayHandle(cellvar, std::cerr, true);

  using PointType = svtkm::Vec3f_64;
  auto pointArray = output.GetCoordinateSystem(0).GetData();
  SVTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points,
                   "Number of output points mismatch");
  for (svtkm::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
  {
    const PointType& p1 = pointArray.GetPortalConstControl().Get(i);
    PointType p2 = svtkm::make_Vec(output_point[i][0], output_point[i][1], output_point[i][2]);
    SVTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
  }

  {
    auto portal = pointvar.GetPortalConstControl();
    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == output_points, "Point field size mismatch.");
    for (svtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(test_equal(portal.Get(i), output_pointvar[i]), "Point field mismatch.");
    }
  }

  {
    auto portal = cellvar.GetPortalConstControl();
    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == 6, "Cell field size mismatch.");
    for (svtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(test_equal(portal.Get(i), output_cellvar[i]), "Cell field mismatch.");
    }
  }
}

int UnitTestVertexClusteringFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestVertexClustering, argc, argv);
}
