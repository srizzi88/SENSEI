//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <ctime>
#include <random>
#include <vector>

namespace DataSetBuilderRectilinearNamespace
{

std::mt19937 g_RandomGenerator;

void ValidateDataSet(const svtkm::cont::DataSet& ds,
                     int dim,
                     svtkm::Id numPoints,
                     svtkm::Id numCells,
                     const svtkm::Bounds& bounds)
{
  //Verify basics..

  SVTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Wrong number of fields.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1, "Wrong number of coordinate systems.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numPoints, "Wrong number of coordinates.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfCells() == numCells, "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    ds.GetPointField("pointvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure the bounds are correct.
  svtkm::Bounds res = ds.GetCoordinateSystem().GetBounds();
  SVTKM_TEST_ASSERT(test_equal(bounds, res), "Bounds of coordinates do not match");
  if (dim == 1)
  {
    svtkm::cont::CellSetStructured<1> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    svtkm::IdComponent shape = cellSet.GetCellShape();
    SVTKM_TEST_ASSERT(shape == svtkm::CELL_SHAPE_LINE, "Wrong element type");
  }
  else if (dim == 2)
  {
    svtkm::cont::CellSetStructured<2> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    svtkm::IdComponent shape = cellSet.GetCellShape();
    SVTKM_TEST_ASSERT(shape == svtkm::CELL_SHAPE_QUAD, "Wrong element type");
  }
  else if (dim == 3)
  {
    svtkm::cont::CellSetStructured<3> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    svtkm::IdComponent shape = cellSet.GetCellShape();
    SVTKM_TEST_ASSERT(shape == svtkm::CELL_SHAPE_HEXAHEDRON, "Wrong element type");
  }
}

template <typename T>
void FillArray(std::vector<T>& arr, svtkm::Id size, svtkm::IdComponent fillMethod)
{
  arr.resize(static_cast<std::size_t>(size));
  arr[0] = T(0);
  for (size_t i = 1; i < static_cast<std::size_t>(size); i++)
  {
    T xi = static_cast<T>(i);

    switch (fillMethod)
    {
      case 0:
        break;
      case 1:
        xi /= static_cast<svtkm::Float32>(size - 1);
        break;
      case 2:
        xi *= 2;
        break;
      case 3:
        xi *= 0.1f;
        break;
      case 4:
        xi *= xi;
        break;
      default:
        SVTKM_TEST_FAIL("Bad internal test state: invalid fill method.");
    }
    arr[i] = xi;
  }
}

template <typename T>
void RectilinearTests()
{
  const svtkm::Id NUM_TRIALS = 10;
  const svtkm::Id MAX_DIM_SIZE = 20;
  const svtkm::Id NUM_FILL_METHODS = 5;

  svtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetFieldAdd dsf;

  std::uniform_int_distribution<svtkm::Id> randomDim(1, MAX_DIM_SIZE);
  std::uniform_int_distribution<svtkm::IdComponent> randomFill(0, NUM_FILL_METHODS - 1);

  for (svtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    std::cout << "Trial " << trial << std::endl;

    svtkm::Id3 dimensions(
      randomDim(g_RandomGenerator), randomDim(g_RandomGenerator), randomDim(g_RandomGenerator));
    std::cout << "Dimensions: " << dimensions << std::endl;

    svtkm::IdComponent fillMethodX = randomFill(g_RandomGenerator);
    svtkm::IdComponent fillMethodY = randomFill(g_RandomGenerator);
    svtkm::IdComponent fillMethodZ = randomFill(g_RandomGenerator);
    std::cout << "Fill methods: [" << fillMethodX << "," << fillMethodY << "," << fillMethodZ << "]"
              << std::endl;

    std::vector<T> xCoordinates;
    std::vector<T> yCoordinates;
    std::vector<T> zCoordinates;
    FillArray(xCoordinates, dimensions[0], fillMethodX);
    FillArray(yCoordinates, dimensions[1], fillMethodY);
    FillArray(zCoordinates, dimensions[2], fillMethodZ);

    svtkm::Id numPoints = 1, numCells = 1;
    svtkm::Bounds bounds(0, 0, 0, 0, 0, 0);
    int ndims = 0;

    std::cout << "1D parameters" << std::endl;
    bounds.X = svtkm::Range(xCoordinates.front(), xCoordinates.back());
    numPoints *= dimensions[0];
    if (dimensions[0] > 1)
    {
      numCells = dimensions[0] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP1D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP1D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC1D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC1D[i] = static_cast<T>(fi * 1.1f);
      }
      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates);
      dsf.AddPointField(dataSet, "pointvar", varP1D);
      dsf.AddCellField(dataSet, "cellvar", varC1D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }

    std::cout << "2D parameters" << std::endl;
    bounds.Y = svtkm::Range(yCoordinates.front(), yCoordinates.back());
    numPoints *= dimensions[1];
    if (dimensions[1] > 1)
    {
      numCells *= dimensions[1] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP2D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP2D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC2D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC2D[i] = static_cast<T>(fi * 1.1f);
      }
      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates);
      dsf.AddPointField(dataSet, "pointvar", varP2D);
      dsf.AddCellField(dataSet, "cellvar", varC2D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with C array" << std::endl;
      dataSet = dataSetBuilder.Create(
        dimensions[0], dimensions[1], &xCoordinates.front(), &yCoordinates.front());
      dsf.AddPointField(dataSet, "pointvar", &varP2D.front(), numPoints);
      dsf.AddCellField(dataSet, "cellvar", &varC2D.front(), numCells);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with ArrayHandle" << std::endl;
      dataSet = dataSetBuilder.Create(svtkm::cont::make_ArrayHandle(xCoordinates),
                                      svtkm::cont::make_ArrayHandle(yCoordinates));
      dsf.AddPointField(dataSet, "pointvar", svtkm::cont::make_ArrayHandle(varP2D));
      dsf.AddCellField(dataSet, "cellvar", svtkm::cont::make_ArrayHandle(varC2D));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }

    std::cout << "3D parameters" << std::endl;
    bounds.Z = svtkm::Range(zCoordinates.front(), zCoordinates.back());
    numPoints *= dimensions[2];
    if (dimensions[2] > 1)
    {
      numCells *= dimensions[2] - 1;
      ndims += 1;
    }
    if (ndims)
    {
      std::vector<T> varP3D(static_cast<unsigned long>(numPoints));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
      {
        float fi = static_cast<float>(i);
        varP3D[i] = static_cast<T>(fi * 1.1f);
      }
      std::vector<T> varC3D(static_cast<unsigned long>(numCells));
      for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
      {
        float fi = static_cast<float>(i);
        varC3D[i] = static_cast<T>(fi * 1.1f);
      }

      std::cout << "  Create with std::vector" << std::endl;
      dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates, zCoordinates);
      dsf.AddPointField(dataSet, "pointvar", varP3D);
      dsf.AddCellField(dataSet, "cellvar", varC3D);
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with C array" << std::endl;
      dataSet = dataSetBuilder.Create(dimensions[0],
                                      dimensions[1],
                                      dimensions[2],
                                      &xCoordinates.front(),
                                      &yCoordinates.front(),
                                      &zCoordinates.front());
      dsf.AddPointField(dataSet, "pointvar", svtkm::cont::make_ArrayHandle(varP3D));
      dsf.AddCellField(dataSet, "cellvar", svtkm::cont::make_ArrayHandle(varC3D));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);

      std::cout << "  Create with ArrayHandle" << std::endl;
      dataSet = dataSetBuilder.Create(svtkm::cont::make_ArrayHandle(xCoordinates),
                                      svtkm::cont::make_ArrayHandle(yCoordinates),
                                      svtkm::cont::make_ArrayHandle(zCoordinates));
      dsf.AddPointField(dataSet, "pointvar", svtkm::cont::make_ArrayHandle(varP3D));
      dsf.AddCellField(dataSet, "cellvar", svtkm::cont::make_ArrayHandle(varC3D));
      ValidateDataSet(dataSet, ndims, numPoints, numCells, bounds);
    }
  }
}

void TestDataSetBuilderRectilinear()
{
  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  RectilinearTests<svtkm::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  RectilinearTests<svtkm::Float64>();
}

} // namespace DataSetBuilderRectilinearNamespace

int UnitTestDataSetBuilderRectilinear(int argc, char* argv[])
{
  using namespace DataSetBuilderRectilinearNamespace;
  return svtkm::cont::testing::Testing::Run(TestDataSetBuilderRectilinear, argc, argv);
}
