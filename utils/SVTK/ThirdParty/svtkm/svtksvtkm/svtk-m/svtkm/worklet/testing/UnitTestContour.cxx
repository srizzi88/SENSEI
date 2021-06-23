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
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/filter/ClipWithImplicitFunction.h>
#include <svtkm/source/Tangle.h>
#include <svtkm/worklet/Contour.h>

namespace svtkm_ut_mc_worklet
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

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArray;
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(0, 1, nCells),
                        cellFieldArray);

  ConnectivityArrayHandle connectivity(
    svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, nCells * HexTraits::NUM_POINTS),
    CubeGridConnectivity(dim));

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(svtkm::cont::Field(
    "distanceToOrigin", svtkm::cont::Field::Association::POINTS, distanceToOrigin));
  dataSet.AddField(svtkm::cont::Field("distanceToOther",
                                     svtkm::cont::Field::Association::POINTS,
                                     svtkm::cont::VariantArrayHandle(distanceToOther)));

  CellSet cellSet;
  cellSet.Fill((dim + 1) * (dim + 1) * (dim + 1), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.SetCellSet(cellSet);

  dataSet.AddField(
    svtkm::cont::Field("cellvar", svtkm::cont::Field::Association::CELL_SET, cellFieldArray));

  return dataSet;
}

} // svtkm_ut_mc_worklet namespace

void TestContourUniformGrid()
{
  std::cout << "Testing Contour worklet on a uniform grid" << std::endl;

  svtkm::Id3 dims(4, 4, 4);
  svtkm::source::Tangle tangle(dims);
  svtkm::cont::DataSet dataSet = tangle.Execute();

  svtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);
  svtkm::cont::ArrayHandle<svtkm::Float32> pointFieldArray;
  dataSet.GetField("nodevar").GetData().CopyTo(pointFieldArray);
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  svtkm::worklet::Contour isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  svtkm::Float32 contourValue = 0.5f;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> verticesArray;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> normalsArray;
  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsArray;

  auto result = isosurfaceFilter.Run(&contourValue,
                                     1,
                                     cellSet,
                                     dataSet.GetCoordinateSystem(),
                                     pointFieldArray,
                                     verticesArray,
                                     normalsArray);

  scalarsArray = isosurfaceFilter.ProcessPointField(pointFieldArray);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = isosurfaceFilter.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  svtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  svtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  svtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  svtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());

  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == 160);

  SVTKM_TEST_ASSERT(verticesArray.GetNumberOfValues() == 480);
}

void TestContourExplicit()
{
  std::cout << "Testing Contour worklet on explicit data" << std::endl;

  using DataSetGenerator = svtkm_ut_mc_worklet::MakeRadiantDataSet;
  using Vec3Handle = svtkm::cont::ArrayHandle<svtkm::Vec3f_32>;
  using DataHandle = svtkm::cont::ArrayHandle<svtkm::Float32>;

  DataSetGenerator dataSetGenerator;

  svtkm::IdComponent Dimension = 10;
  svtkm::Float32 contourValue = svtkm::Float32(.45);

  svtkm::cont::DataSet dataSet = dataSetGenerator.Make3DRadiantDataSet(Dimension);

  DataSetGenerator::CellSet cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  svtkm::cont::Field contourField = dataSet.GetField("distanceToOrigin");
  DataSetGenerator::DataArrayHandle contourArray;
  contourField.GetData().CopyTo(contourArray);
  Vec3Handle vertices;
  Vec3Handle normals;

  svtkm::worklet::Contour Contour;
  Contour.SetMergeDuplicatePoints(false);

  auto result = Contour.Run(
    &contourValue, 1, cellSet, dataSet.GetCoordinateSystem(), contourArray, vertices, normals);

  DataHandle scalars;

  svtkm::cont::Field projectedField = dataSet.GetField("distanceToOther");

  DataSetGenerator::DataArrayHandle projectedArray;
  projectedField.GetData().CopyTo(projectedArray);

  scalars = Contour.ProcessPointField(projectedArray);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArray;
  dataSet.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = Contour.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  svtkm::cont::printSummary_ArrayHandle(vertices, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  svtkm::cont::printSummary_ArrayHandle(normals, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  svtkm::cont::printSummary_ArrayHandle(scalars, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  svtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());
  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == 824);
  SVTKM_TEST_ASSERT(test_equal(vertices.GetNumberOfValues(), 2472));
  SVTKM_TEST_ASSERT(test_equal(normals.GetNumberOfValues(), 2472));
  SVTKM_TEST_ASSERT(test_equal(scalars.GetNumberOfValues(), 2472));
}

void TestContourClipped()
{
  std::cout << "Testing Contour worklet on a clipped uniform grid" << std::endl;

  svtkm::Id3 dims(4, 4, 4);
  svtkm::source::Tangle tangle(dims);
  svtkm::cont::DataSet dataSet = tangle.Execute();

  svtkm::Plane plane(svtkm::make_Vec(0.51, 0.51, 0.51), svtkm::make_Vec(1, 1, 1));
  svtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(svtkm::cont::make_ImplicitFunctionHandle(plane));
  svtkm::cont::DataSet clipped = clip.Execute(dataSet);

  svtkm::cont::CellSetExplicit<> cellSet;
  clipped.GetCellSet().CopyTo(cellSet);
  svtkm::cont::ArrayHandle<svtkm::Float32> pointFieldArray;
  clipped.GetField("nodevar").GetData().CopyTo(pointFieldArray);
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArray;
  clipped.GetField("cellvar").GetData().CopyTo(cellFieldArray);

  svtkm::Float32 contourValue = 0.5f;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> verticesArray;
  svtkm::cont::ArrayHandle<svtkm::Vec3f_32> normalsArray;
  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsArray;

  svtkm::worklet::Contour isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  auto result = isosurfaceFilter.Run(&contourValue,
                                     1,
                                     cellSet,
                                     clipped.GetCoordinateSystem(),
                                     pointFieldArray,
                                     verticesArray,
                                     normalsArray);

  scalarsArray = isosurfaceFilter.ProcessPointField(pointFieldArray);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArrayOut;
  cellFieldArrayOut = isosurfaceFilter.ProcessCellField(cellFieldArray);

  std::cout << "vertices: ";
  svtkm::cont::printSummary_ArrayHandle(verticesArray, std::cout);
  std::cout << std::endl;
  std::cout << "normals: ";
  svtkm::cont::printSummary_ArrayHandle(normalsArray, std::cout);
  std::cout << std::endl;
  std::cout << "scalars: ";
  svtkm::cont::printSummary_ArrayHandle(scalarsArray, std::cout);
  std::cout << std::endl;
  std::cout << "cell field: ";
  svtkm::cont::printSummary_ArrayHandle(cellFieldArrayOut, std::cout);
  std::cout << std::endl;

  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == cellFieldArrayOut.GetNumberOfValues());
  SVTKM_TEST_ASSERT(result.GetNumberOfCells() == 170);
  SVTKM_TEST_ASSERT(verticesArray.GetNumberOfValues() == 510);
  SVTKM_TEST_ASSERT(normalsArray.GetNumberOfValues() == 510);
  SVTKM_TEST_ASSERT(scalarsArray.GetNumberOfValues() == 510);
}

void TestContour()
{
  TestContourUniformGrid();
  TestContourExplicit();
  TestContourClipped();
}

int UnitTestContour(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestContour, argc, argv);
}
