//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/filter/CellAverage.h>
#include <svtkm/filter/Contour.h>
#include <svtkm/filter/SplitSharpEdges.h>
#include <svtkm/filter/SurfaceNormals.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/source/Wavelet.h>

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

const std::vector<svtkm::Id> expectedConnectivityArray91{ 0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                         3, 0, 4, 7, 4, 5, 6, 7, 0, 3, 2, 1 };
const std::vector<svtkm::FloatDefault> expectedPointvar{ 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f,
                                                        70.3f, 80.3f, 10.1f, 10.1f, 20.1f, 20.1f,
                                                        30.2f, 30.2f, 40.2f, 40.2f, 50.3f, 50.3f,
                                                        60.3f, 60.3f, 70.3f, 70.3f, 80.3f, 80.3f };


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

svtkm::cont::DataSet Make3DWavelet()
{

  svtkm::source::Wavelet wavelet({ -25 }, { 25 });
  wavelet.SetFrequency({ 60, 30, 40 });
  wavelet.SetMagnitude({ 5 });

  svtkm::cont::DataSet result = wavelet.Execute();
  return result;
}


void TestSplitSharpEdgesFilterSplitEveryEdge(svtkm::cont::DataSet& simpleCubeWithSN,
                                             svtkm::filter::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Split every edge
  svtkm::FloatDefault featureAngle = 89.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", svtkm::cont::Field::Association::CELL_SET);
  svtkm::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetData();
  auto newCoordsP = newCoords.GetPortalConstControl();
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().CopyTo(newPointvarField);

  for (svtkm::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                     "result value does not match expected value");
    SVTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                     "result value does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.GetPortalConstControl();
  for (svtkm::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<svtkm::Id>(i)),
                                expectedPointvar[static_cast<unsigned long>(i)]),
                     "point field array result does not match expected value");
  }
}

void TestSplitSharpEdgesFilterNoSplit(svtkm::cont::DataSet& simpleCubeWithSN,
                                      svtkm::filter::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Do nothing
  svtkm::FloatDefault featureAngle = 91.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", svtkm::cont::Field::Association::CELL_SET);
  svtkm::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetData();
  svtkm::cont::CellSetExplicit<>& newCellset =
    result.GetCellSet().Cast<svtkm::cont::CellSetExplicit<>>();
  auto newCoordsP = newCoords.GetPortalConstControl();
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().CopyTo(newPointvarField);

  for (svtkm::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
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
  for (svtkm::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(connectivityArrayPortal.Get(static_cast<svtkm::Id>(i)) ==
                       expectedConnectivityArray91[static_cast<unsigned long>(i)],
                     "connectivity array result does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.GetPortalConstControl();
  for (svtkm::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<svtkm::Id>(i)),
                                expectedPointvar[static_cast<unsigned long>(i)]),
                     "point field array result does not match expected value");
  }
}

void TestWithExplicitData()
{
  svtkm::cont::DataSet simpleCube = Make3DExplicitSimpleCube();
  // Generate surface normal field
  svtkm::filter::SurfaceNormals surfaceNormalsFilter;
  surfaceNormalsFilter.SetGenerateCellNormals(true);
  svtkm::cont::DataSet simpleCubeWithSN = surfaceNormalsFilter.Execute(simpleCube);
  SVTKM_TEST_ASSERT(simpleCubeWithSN.HasCellField("Normals"), "Cell normals missing.");
  SVTKM_TEST_ASSERT(simpleCubeWithSN.HasPointField("pointvar"), "point field pointvar missing.");


  svtkm::filter::SplitSharpEdges splitSharpEdgesFilter;

  TestSplitSharpEdgesFilterSplitEveryEdge(simpleCubeWithSN, splitSharpEdgesFilter);
  TestSplitSharpEdgesFilterNoSplit(simpleCubeWithSN, splitSharpEdgesFilter);
}

struct SplitSharpTestPolicy : public svtkm::filter::PolicyBase<SplitSharpTestPolicy>
{
  using StructuredCellSetList = svtkm::List<svtkm::cont::CellSetStructured<3>>;
  using UnstructuredCellSetList = svtkm::List<svtkm::cont::CellSetSingleType<>>;
  using AllCellSetList = svtkm::ListAppend<StructuredCellSetList, UnstructuredCellSetList>;
  using FieldTypeList = svtkm::List<svtkm::FloatDefault, svtkm::Vec3f>;
};


void TestWithStructuredData()
{
  // Generate a wavelet:
  svtkm::cont::DataSet dataSet = Make3DWavelet();

  // Cut a contour:
  svtkm::filter::Contour contour;
  contour.SetActiveField("scalars", svtkm::cont::Field::Association::POINTS);
  contour.SetNumberOfIsoValues(1);
  contour.SetIsoValue(192);
  contour.SetMergeDuplicatePoints(true);
  contour.SetGenerateNormals(true);
  contour.SetComputeFastNormalsForStructured(true);
  contour.SetNormalArrayName("normals");
  dataSet = contour.Execute(dataSet);

  // Compute cell normals:
  svtkm::filter::CellAverage cellNormals;
  cellNormals.SetActiveField("normals", svtkm::cont::Field::Association::POINTS);
  dataSet = cellNormals.Execute(dataSet);

  // Split sharp edges:
  std::cout << dataSet.GetNumberOfCells() << std::endl;
  std::cout << dataSet.GetNumberOfPoints() << std::endl;
  svtkm::filter::SplitSharpEdges split;
  split.SetActiveField("normals", svtkm::cont::Field::Association::CELL_SET);
  dataSet = split.Execute(dataSet, SplitSharpTestPolicy{});
}


void TestSplitSharpEdgesFilter()
{
  TestWithExplicitData();
  TestWithStructuredData();
}

} // anonymous namespace

int UnitTestSplitSharpEdgesFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestSplitSharpEdgesFilter, argc, argv);
}
