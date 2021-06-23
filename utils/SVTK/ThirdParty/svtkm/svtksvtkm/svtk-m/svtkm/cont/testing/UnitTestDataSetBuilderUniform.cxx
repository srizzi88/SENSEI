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
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <random>
#include <time.h>
#include <vector>

namespace DataSetBuilderUniformNamespace
{

std::mt19937 g_RandomGenerator;

void ValidateDataSet(const svtkm::cont::DataSet& ds,
                     int dim,
                     svtkm::Id numPoints,
                     svtkm::Id numCells,
                     svtkm::Bounds bounds)
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

  //Make sure bounds are correct.
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
svtkm::Range FillMethod(svtkm::IdComponent method, svtkm::Id dimensionSize, T& origin, T& spacing)
{
  switch (method)
  {
    case 0:
      origin = 0;
      spacing = 1;
      break;
    case 1:
      origin = 0;
      spacing = static_cast<T>(1.0 / static_cast<double>(dimensionSize));
      break;
    case 2:
      origin = 0;
      spacing = 2;
      break;
    case 3:
      origin = static_cast<T>(-(dimensionSize - 1));
      spacing = 1;
      break;
    case 4:
      origin = static_cast<T>(2.780941);
      spacing = static_cast<T>(182.381901);
      break;
    default:
      origin = 0;
      spacing = 0;
      break;
  }

  return svtkm::Range(origin, origin + static_cast<T>(dimensionSize - 1) * spacing);
}

svtkm::Range& GetRangeByIndex(svtkm::Bounds& bounds, int comp)
{
  SVTKM_ASSERT(comp >= 0 && comp < 3);
  switch (comp)
  {
    case 0:
      return bounds.X;
    case 1:
      return bounds.Y;
    default:
      return bounds.Z;
  }
}

template <typename T>
void UniformTests()
{
  const svtkm::Id NUM_TRIALS = 10;
  const svtkm::Id MAX_DIM_SIZE = 20;
  const svtkm::Id NUM_FILL_METHODS = 5;

  svtkm::cont::DataSetBuilderUniform dataSetBuilder;
  svtkm::cont::DataSetFieldAdd dsf;

  std::uniform_int_distribution<svtkm::Id> randomDim(2, MAX_DIM_SIZE);
  std::uniform_int_distribution<svtkm::IdComponent> randomFill(0, NUM_FILL_METHODS - 1);
  std::uniform_int_distribution<svtkm::IdComponent> randomAxis(0, 2);

  for (svtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    std::cout << "Trial " << trial << std::endl;

    svtkm::Id3 dimensions(
      randomDim(g_RandomGenerator), randomDim(g_RandomGenerator), randomDim(g_RandomGenerator));

    svtkm::IdComponent fillMethodX = randomFill(g_RandomGenerator);
    svtkm::IdComponent fillMethodY = randomFill(g_RandomGenerator);
    svtkm::IdComponent fillMethodZ = randomFill(g_RandomGenerator);
    std::cout << "Fill methods: [" << fillMethodX << "," << fillMethodY << "," << fillMethodZ << "]"
              << std::endl;

    svtkm::Vec<T, 3> origin;
    svtkm::Vec<T, 3> spacing;
    svtkm::Range ranges[3];
    ranges[0] = FillMethod(fillMethodX, dimensions[0], origin[0], spacing[0]);
    ranges[1] = FillMethod(fillMethodY, dimensions[1], origin[1], spacing[1]);
    ranges[2] = FillMethod(fillMethodZ, dimensions[2], origin[2], spacing[2]);

    std::cout << "3D cellset" << std::endl;
    {
      svtkm::Id3 dims = dimensions;
      svtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dims[0] * dims[1] * dims[2];
      svtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);
    }

    std::cout << "2D cellset, 2D parameters" << std::endl;
    {
      svtkm::Id2 dims(dimensions[0], dimensions[1]);
      svtkm::Bounds bounds(ranges[0], ranges[1], svtkm::Range(0, 0));
      svtkm::Vec<T, 2> org(origin[0], origin[1]);
      svtkm::Vec<T, 2> spc(spacing[0], spacing[1]);

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << org << std::endl;
      std::cout << "\tspacing: " << spc << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dims[0] * dims[1];
      svtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, org, spc);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);
    }

    std::cout << "2D cellset, 3D parameters" << std::endl;
    {
      svtkm::Id3 dims = dimensions;
      svtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      int x = randomAxis(g_RandomGenerator);
      dims[x] = 1;
      GetRangeByIndex(bounds, x).Max = ranges[x].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dims[(x + 1) % 3] * dims[(x + 2) % 3];
      svtkm::Id numCells = (dims[(x + 1) % 3] - 1) * (dims[(x + 2) % 3] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 1D parameters" << std::endl;
    {
      svtkm::Bounds bounds(ranges[0], svtkm::Range(0, 0), svtkm::Range(0, 0));

      std::cout << "\tdimensions: " << dimensions[0] << std::endl;
      std::cout << "\toriging: " << origin[0] << std::endl;
      std::cout << "\tspacing: " << spacing[0] << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dimensions[0];
      svtkm::Id numCells = dimensions[0] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dimensions[0], origin[0], spacing[0]);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 2D parameters" << std::endl;
    {
      svtkm::Id2 dims(dimensions[0], dimensions[1]);
      svtkm::Bounds bounds(ranges[0], ranges[1], svtkm::Range(0, 0));
      svtkm::Vec<T, 2> org(origin[0], origin[1]);
      svtkm::Vec<T, 2> spc(spacing[0], spacing[1]);

      int x = randomAxis(g_RandomGenerator) % 2;
      dims[x] = 1;
      GetRangeByIndex(bounds, x).Max = ranges[x].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << org << std::endl;
      std::cout << "\tspacing: " << spc << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dims[(x + 1) % 2];
      svtkm::Id numCells = dims[(x + 1) % 2] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, org, spc);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 3D parameters" << std::endl;
    {
      svtkm::Id3 dims = dimensions;
      svtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      int x = randomAxis(g_RandomGenerator);
      int x1 = (x + 1) % 3;
      int x2 = (x + 2) % 3;
      dims[x1] = dims[x2] = 1;
      GetRangeByIndex(bounds, x1).Max = ranges[x1].Min;
      GetRangeByIndex(bounds, x2).Max = ranges[x2].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      svtkm::Id numPoints = dims[x];
      svtkm::Id numCells = dims[x] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      svtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }
  }
}

void TestDataSetBuilderUniform()
{
  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  UniformTests<svtkm::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  UniformTests<svtkm::Float64>();
}

} // namespace DataSetBuilderUniformNamespace

int UnitTestDataSetBuilderUniform(int argc, char* argv[])
{
  using namespace DataSetBuilderUniformNamespace;
  return svtkm::cont::testing::Testing::Run(TestDataSetBuilderUniform, argc, argv);
}
