//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/SplitSharpEdges.h>
#include <svtkm/worklet/SurfaceNormals.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace
{

using NormalsArrayHandle = svtkm::cont::ArrayHandle<svtkm::Vec3f>;

const svtkm::Vec3f expectedCoords[24] = {
  { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 },
  { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 },
  { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }
};

const std::vector<svtkm::FloatDefault> expectedPointvar{ 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f,
                                                        70.3f, 80.3f, 10.1f, 10.1f, 20.1f, 20.1f,
                                                        30.2f, 30.2f, 40.2f, 40.2f, 50.3f, 50.3f,
                                                        60.3f, 60.3f, 70.3f, 70.3f, 80.3f, 80.3f };

const std::vector<svtkm::Id> expectedConnectivityArray91{ 0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                         3, 0, 4, 7, 4, 5, 6, 7, 0, 3, 2, 1 };

svtkm::cont::DataSet Make3DExplicitSimpleCube()
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 8;
  const int nCells = 6;
  using CoordType = svtkm::Vec3f;
  std::vector<CoordType> coords = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };

  //Connectivity
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numIndices;
  for (size_t i = 0; i < 6; i++)
  {
    shapes.push_back(svtkm::CELL_SHAPE_QUAD);
    numIndices.push_back(4);
  }


  std::vector<svtkm::Id> conn;
  // Down face
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(4);
  // Right face
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  // Top face
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(6);
  // Left face
  conn.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(7);
  // Front face
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  // Back face
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(1);

  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numIndices, conn, "coordinates");

  svtkm::FloatDefault vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f, 70.3f, 80.3f };
  svtkm::FloatDefault cellvar[nCells] = { 100.1f, 200.2f, 300.3f, 400.4f, 500.5f, 600.6f };

  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

void TestSplitSharpEdgesSplitEveryEdge(svtkm::cont::DataSet& simpleCube,
                                       NormalsArrayHandle& faceNormals,
                                       svtkm::worklet::SplitSharpEdges& splitSharpEdges)

{ // Split every edge
  svtkm::FloatDefault featureAngle = 89.0;

  svtkm::cont::ArrayHandle<svtkm::Vec3f> newCoords;
  svtkm::cont::CellSetExplicit<> newCellset;

  splitSharpEdges.Run(simpleCube.GetCellSet(),
                      featureAngle,
                      faceNormals,
                      simpleCube.GetCoordinateSystem().GetData(),
                      newCoords,
                      newCellset);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> pointvar;
  simpleCube.GetPointField("pointvar").GetData().CopyTo(pointvar);
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> newPointFields =
    splitSharpEdges.ProcessPointField(pointvar);
  SVTKM_TEST_ASSERT(newCoords.GetNumberOfValues() == 24,
                   "new coordinates"
                   " number is wrong");

  auto newCoordsP = newCoords.GetPortalConstControl();
  for (svtkm::Id i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[svtkm::IdComponent(i)][0]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[svtkm::IdComponent(i)][1]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[svtkm::IdComponent(i)][2]),
                     "result value does not match expected value");
  }

  auto newPointFieldsPortal = newPointFields.GetPortalConstControl();
  for (int i = 0; i < newPointFields.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(
      test_equal(newPointFieldsPortal.Get(i), expectedPointvar[static_cast<unsigned long>(i)]),
      "point field array result does not match expected value");
  }
}

void TestSplitSharpEdgesNoSplit(svtkm::cont::DataSet& simpleCube,
                                NormalsArrayHandle& faceNormals,
                                svtkm::worklet::SplitSharpEdges& splitSharpEdges)

{ // Do nothing
  svtkm::FloatDefault featureAngle = 91.0;

  svtkm::cont::ArrayHandle<svtkm::Vec3f> newCoords;
  svtkm::cont::CellSetExplicit<> newCellset;

  splitSharpEdges.Run(simpleCube.GetCellSet(),
                      featureAngle,
                      faceNormals,
                      simpleCube.GetCoordinateSystem().GetData(),
                      newCoords,
                      newCellset);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> pointvar;
  simpleCube.GetPointField("pointvar").GetData().CopyTo(pointvar);
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> newPointFields =
    splitSharpEdges.ProcessPointField(pointvar);
  SVTKM_TEST_ASSERT(newCoords.GetNumberOfValues() == 8,
                   "new coordinates"
                   " number is wrong");

  auto newCoordsP = newCoords.GetPortalConstControl();
  for (int i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                     "result value does not match expected value");
  }

  const auto& connectivityArray = newCellset.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                                  svtkm::TopologyElementTagPoint());
  auto connectivityArrayPortal = connectivityArray.GetPortalConstControl();
  for (int i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(connectivityArrayPortal.Get(i),
                                expectedConnectivityArray91[static_cast<unsigned long>(i)]),
                     "connectivity array result does not match expected value");
  }

  auto newPointFieldsPortal = newPointFields.GetPortalConstControl();
  for (int i = 0; i < newPointFields.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(
      test_equal(newPointFieldsPortal.Get(i), expectedPointvar[static_cast<unsigned long>(i)]),
      "point field array result does not match expected value");
  }
}

void TestSplitSharpEdges()
{
  svtkm::cont::DataSet simpleCube = Make3DExplicitSimpleCube();
  NormalsArrayHandle faceNormals;
  svtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.Run(simpleCube.GetCellSet(), simpleCube.GetCoordinateSystem().GetData(), faceNormals);

  svtkm::worklet::SplitSharpEdges splitSharpEdges;

  TestSplitSharpEdgesSplitEveryEdge(simpleCube, faceNormals, splitSharpEdges);
  TestSplitSharpEdgesNoSplit(simpleCube, faceNormals, splitSharpEdges);
}

} // anonymous namespace

int UnitTestSplitSharpEdges(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestSplitSharpEdges, argc, argv);
}
