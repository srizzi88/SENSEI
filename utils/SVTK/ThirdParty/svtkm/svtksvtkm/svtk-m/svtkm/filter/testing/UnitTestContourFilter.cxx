//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Math.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/CleanGrid.h>

#include <svtkm/filter/Contour.h>
#include <svtkm/filter/Contour.hxx>
#include <svtkm/source/Tangle.h>

namespace svtkm_ut_mc_filter
{
class EuclideanNorm
{
public:
  SVTKM_EXEC_CONT
  EuclideanNorm()
    : Reference(0., 0., 0.)
  {
  }
  SVTKM_EXEC_CONT
  EuclideanNorm(svtkm::Vec3f_32 reference)
    : Reference(reference)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Float32 operator()(svtkm::Vec3f_32 v) const
  {
    svtkm::Vec3f_32 d(
      v[0] - this->Reference[0], v[1] - this->Reference[1], v[2] - this->Reference[2]);
    return svtkm::Magnitude(d);
  }

private:
  svtkm::Vec3f_32 Reference;
};

class CubeGridConnectivity
{
public:
  SVTKM_EXEC_CONT
  CubeGridConnectivity()
    : Dimension(1)
    , DimSquared(1)
    , DimPlus1Squared(4)
  {
  }
  SVTKM_EXEC_CONT
  CubeGridConnectivity(svtkm::Id dim)
    : Dimension(dim)
    , DimSquared(dim * dim)
    , DimPlus1Squared((dim + 1) * (dim + 1))
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id vertex) const
  {
    using HexTag = svtkm::CellShapeTagHexahedron;
    using HexTraits = svtkm::CellTraits<HexTag>;

    svtkm::Id cellId = vertex / HexTraits::NUM_POINTS;
    svtkm::Id localId = vertex % HexTraits::NUM_POINTS;
    svtkm::Id globalId =
      (cellId + cellId / this->Dimension + (this->Dimension + 1) * (cellId / (this->DimSquared)));

    switch (localId)
    {
      case 0:
        break;
      case 1:
        globalId += 1;
        break;
      case 2:
        globalId += this->Dimension + 2;
        break;
      case 3:
        globalId += this->Dimension + 1;
        break;
      case 4:
        globalId += this->DimPlus1Squared;
        break;
      case 5:
        globalId += this->DimPlus1Squared + 1;
        break;
      case 6:
        globalId += this->Dimension + this->DimPlus1Squared + 2;
        break;
      case 7:
        globalId += this->Dimension + this->DimPlus1Squared + 1;
        break;
    }

    return globalId;
  }

private:
  svtkm::Id Dimension;
  svtkm::Id DimSquared;
  svtkm::Id DimPlus1Squared;
};

class MakeRadiantDataSet
{
public:
  using CoordinateArrayHandle = svtkm::cont::ArrayHandleUniformPointCoordinates;
  using DataArrayHandle =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandleUniformPointCoordinates, EuclideanNorm>;
  using ConnectivityArrayHandle =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandleCounting<svtkm::Id>,
                                     CubeGridConnectivity>;

  using CellSet = svtkm::cont::CellSetSingleType<
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandleCounting<svtkm::Id>,
                                     CubeGridConnectivity>::StorageTag>;

  svtkm::cont::DataSet Make3DRadiantDataSet(svtkm::IdComponent dim = 5);
};

class PolicyRadiantDataSet : public svtkm::filter::PolicyBase<PolicyRadiantDataSet>
{
public:
  using TypeListRadiantCellSetTypes = svtkm::List<MakeRadiantDataSet::CellSet>;

  using AllCellSetList = TypeListRadiantCellSetTypes;
};

inline svtkm::cont::DataSet MakeRadiantDataSet::Make3DRadiantDataSet(svtkm::IdComponent dim)
{
  // create a cube from -.5 to .5 in x,y,z, consisting of <dim> cells on each
  // axis, with point values equal to the Euclidean distance from the origin.

  svtkm::cont::DataSet dataSet;

  using HexTag = svtkm::CellShapeTagHexahedron;
  using HexTraits = svtkm::CellTraits<HexTag>;

  using CoordType = svtkm::Vec3f_32;

  const svtkm::IdComponent nCells = dim * dim * dim;

  svtkm::Float32 spacing = svtkm::Float32(1. / dim);
  CoordinateArrayHandle coordinates(svtkm::Id3(dim + 1, dim + 1, dim + 1),
                                    CoordType(-.5, -.5, -.5),
                                    CoordType(spacing, spacing, spacing));

  DataArrayHandle distanceToOrigin(coordinates);
  DataArrayHandle distanceToOther(coordinates, EuclideanNorm(CoordType(1., 1., 1.)));

  ConnectivityArrayHandle connectivity(
    svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, nCells * HexTraits::NUM_POINTS),
    CubeGridConnectivity(dim));

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(svtkm::cont::Field(
    "distanceToOrigin", svtkm::cont::Field::Association::POINTS, distanceToOrigin));
  dataSet.AddField(
    svtkm::cont::Field("distanceToOther", svtkm::cont::Field::Association::POINTS, distanceToOther));

  CellSet cellSet;
  cellSet.Fill(coordinates.GetNumberOfValues(), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.SetCellSet(cellSet);

  return dataSet;
}

void TestContourUniformGrid()
{
  std::cout << "Testing Contour filter on a uniform grid" << std::endl;

  svtkm::Id3 dims(4, 4, 4);
  svtkm::source::Tangle tangle(dims);
  svtkm::cont::DataSet dataSet = tangle.Execute();

  svtkm::filter::Contour mc;

  mc.SetGenerateNormals(true);
  mc.SetIsoValue(0, 0.5);
  mc.SetActiveField("nodevar");
  mc.SetFieldsToPass(svtkm::filter::FieldSelection::MODE_NONE);

  auto result = mc.Execute(dataSet);
  {
    SVTKM_TEST_ASSERT(result.GetNumberOfCoordinateSystems() == 1,
                     "Wrong number of coordinate systems in the output dataset");
    //since normals is on we have one field
    SVTKM_TEST_ASSERT(result.GetNumberOfFields() == 1,
                     "Wrong number of fields in the output dataset");
  }

  // let's execute with mapping fields.
  mc.SetFieldsToPass("nodevar");
  result = mc.Execute(dataSet);
  {
    const bool isMapped = result.HasField("nodevar");
    SVTKM_TEST_ASSERT(isMapped, "mapping should pass");

    SVTKM_TEST_ASSERT(result.GetNumberOfFields() == 2,
                     "Wrong number of fields in the output dataset");

    svtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();
    svtkm::cont::DynamicCellSet dcells = result.GetCellSet();
    using CellSetType = svtkm::cont::CellSetSingleType<>;
    const CellSetType& cells = dcells.Cast<CellSetType>();

    //verify that the number of points is correct (72)
    //verify that the number of cells is correct (160)
    SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 72,
                     "Should have less coordinates than the unmerged version");
    SVTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
  }

  //Now try with vertex merging disabled
  mc.SetMergeDuplicatePoints(false);
  mc.SetFieldsToPass(svtkm::filter::FieldSelection::MODE_ALL);
  result = mc.Execute(dataSet);
  {
    svtkm::cont::CoordinateSystem coords = result.GetCoordinateSystem();

    SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 480,
                     "Should have less coordinates than the unmerged version");

    //verify that the number of cells is correct (160)
    svtkm::cont::DynamicCellSet dcells = result.GetCellSet();

    using CellSetType = svtkm::cont::CellSetSingleType<>;
    const CellSetType& cells = dcells.Cast<CellSetType>();
    SVTKM_TEST_ASSERT(cells.GetNumberOfCells() == 160, "");
  }
}

void TestContourCustomPolicy()
{
  std::cout << "Testing Contour filter with custom field and cellset" << std::endl;

  using DataSetGenerator = MakeRadiantDataSet;
  DataSetGenerator dataSetGenerator;

  const svtkm::IdComponent Dimension = 10;
  svtkm::cont::DataSet dataSet = dataSetGenerator.Make3DRadiantDataSet(Dimension);

  svtkm::filter::Contour mc;

  mc.SetGenerateNormals(false);
  mc.SetIsoValue(0, 0.45);
  mc.SetIsoValue(1, 0.45);
  mc.SetIsoValue(2, 0.45);
  mc.SetIsoValue(3, 0.45);

  //We specify a custom execution policy here, since the "distanceToOrigin" is a
  //custom field type
  mc.SetActiveField("distanceToOrigin");
  mc.SetFieldsToPass({ "distanceToOrigin", "distanceToOther" });
  svtkm::cont::DataSet outputData = mc.Execute(dataSet, PolicyRadiantDataSet{});

  SVTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");

  svtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();
  SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == (414 * 4), "Should have some coordinates");
}


svtkm::cont::DataSet MakeNormalsTestDataSet()
{
  svtkm::cont::DataSetBuilderUniform dsb;
  svtkm::Id3 dimensions(3, 4, 4);
  svtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  svtkm::cont::DataSetFieldAdd dsf;
  const int nVerts = 48;
  svtkm::Float32 vars[nVerts] = { 60.764f,  107.555f, 80.524f,  63.639f,  131.087f, 83.4f,
                                 98.161f,  165.608f, 117.921f, 37.353f,  84.145f,  57.114f,
                                 95.202f,  162.649f, 114.962f, 115.896f, 215.56f,  135.657f,
                                 150.418f, 250.081f, 170.178f, 71.791f,  139.239f, 91.552f,
                                 95.202f,  162.649f, 114.962f, 115.896f, 215.56f,  135.657f,
                                 150.418f, 250.081f, 170.178f, 71.791f,  139.239f, 91.552f,
                                 60.764f,  107.555f, 80.524f,  63.639f,  131.087f, 83.4f,
                                 98.161f,  165.608f, 117.921f, 37.353f,  84.145f,  57.114f };

  //Set point and cell scalar
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);

  return dataSet;
}

void TestNormals(const svtkm::cont::DataSet& dataset, bool structured)
{
  const svtkm::Id numVerts = 16;

  //Calculated using PointGradient
  const svtkm::Vec3f hq_ug[numVerts] = {
    { 0.1510f, 0.6268f, 0.7644f },   { 0.1333f, -0.3974f, 0.9079f },
    { 0.1626f, 0.7642f, 0.6242f },   { 0.3853f, 0.6643f, 0.6405f },
    { -0.1337f, 0.7136f, 0.6876f },  { 0.7705f, -0.4212f, 0.4784f },
    { -0.7360f, -0.4452f, 0.5099f }, { 0.1234f, -0.8871f, 0.4448f },
    { 0.1626f, 0.7642f, -0.6242f },  { 0.3853f, 0.6643f, -0.6405f },
    { -0.1337f, 0.7136f, -0.6876f }, { 0.1510f, 0.6268f, -0.7644f },
    { 0.7705f, -0.4212f, -0.4784f }, { -0.7360f, -0.4452f, -0.5099f },
    { 0.1234f, -0.8871f, -0.4448f }, { 0.1333f, -0.3974f, -0.9079f }
  };

  //Calculated using StructuredPointGradient
  const svtkm::Vec3f hq_sg[numVerts] = {
    { 0.151008f, 0.626778f, 0.764425f },   { 0.133328f, -0.397444f, 0.907889f },
    { 0.162649f, 0.764163f, 0.624180f },   { 0.385327f, 0.664323f, 0.640467f },
    { -0.133720f, 0.713645f, 0.687626f },  { 0.770536f, -0.421248f, 0.478356f },
    { -0.736036f, -0.445244f, 0.509910f }, { 0.123446f, -0.887088f, 0.444788f },
    { 0.162649f, 0.764163f, -0.624180f },  { 0.385327f, 0.664323f, -0.640467f },
    { -0.133720f, 0.713645f, -0.687626f }, { 0.151008f, 0.626778f, -0.764425f },
    { 0.770536f, -0.421248f, -0.478356f }, { -0.736036f, -0.445244f, -0.509910f },
    { 0.123446f, -0.887088f, -0.444788f }, { 0.133328f, -0.397444f, -0.907889f }
  };

  //Calculated using normals of the output triangles
  const svtkm::Vec3f fast[numVerts] = {
    { -0.1351f, 0.4377f, 0.8889f },  { 0.2863f, -0.1721f, 0.9426f },
    { 0.3629f, 0.8155f, 0.4509f },   { 0.8486f, 0.3560f, 0.3914f },
    { -0.8315f, 0.4727f, 0.2917f },  { 0.9395f, -0.2530f, 0.2311f },
    { -0.9105f, -0.0298f, 0.4124f }, { -0.1078f, -0.9585f, 0.2637f },
    { -0.2538f, 0.8534f, -0.4553f }, { 0.8953f, 0.3902f, -0.2149f },
    { -0.8295f, 0.4188f, -0.3694f }, { 0.2434f, 0.4297f, -0.8695f },
    { 0.8951f, -0.1347f, -0.4251f }, { -0.8467f, -0.4258f, -0.3191f },
    { 0.2164f, -0.9401f, -0.2635f }, { -0.1589f, -0.1642f, -0.9735f }
  };

  svtkm::cont::ArrayHandle<svtkm::Vec3f> normals;

  svtkm::filter::Contour mc;
  mc.SetIsoValue(0, 200);
  mc.SetGenerateNormals(true);

  // Test default normals generation: high quality for structured, fast for unstructured.
  auto expected = structured ? hq_sg : fast;

  mc.SetActiveField("pointvar");
  auto result = mc.Execute(dataset);
  result.GetField("normals").GetData().CopyTo(normals);
  SVTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");
  for (svtkm::Id i = 0; i < numVerts; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(normals.GetPortalConstControl().Get(i), expected[i], 0.001),
                     "Result (",
                     normals.GetPortalConstControl().Get(i),
                     ") does not match expected value (",
                     expected[i],
                     ") vert ",
                     i);
  }

  // Test the other normals generation method
  if (structured)
  {
    mc.SetComputeFastNormalsForStructured(true);
    expected = fast;
  }
  else
  {
    mc.SetComputeFastNormalsForUnstructured(false);
    expected = hq_ug;
  }

  result = mc.Execute(dataset);
  result.GetField("normals").GetData().CopyTo(normals);
  SVTKM_TEST_ASSERT(normals.GetNumberOfValues() == numVerts,
                   "Wrong number of values in normals field");
  for (svtkm::Id i = 0; i < numVerts; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(normals.GetPortalConstControl().Get(i), expected[i], 0.001),
                     "Result does not match expected values");
  }
}

void TestContourNormals()
{
  std::cout << "Testing Contour normals generation" << std::endl;

  std::cout << "\tStructured dataset\n";
  svtkm::cont::DataSet dataset = MakeNormalsTestDataSet();
  TestNormals(dataset, true);

  std::cout << "\tUnstructured dataset\n";
  svtkm::filter::CleanGrid makeUnstructured;
  makeUnstructured.SetCompactPointFields(false);
  makeUnstructured.SetMergePoints(false);
  makeUnstructured.SetFieldsToPass("pointvar");
  auto result = makeUnstructured.Execute(dataset);
  TestNormals(result, false);
}

void TestContourFilter()
{
  TestContourUniformGrid();
  TestContourCustomPolicy();
  TestContourNormals();
}

} // anonymous namespace

int UnitTestContourFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(svtkm_ut_mc_filter::TestContourFilter, argc, argv);
}
