//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/Pathline.h>
#include <svtkm/filter/Streamline.h>

namespace
{
svtkm::cont::DataSet CreateDataSet(const svtkm::Id3& dims, const svtkm::Vec3f& vec)
{
  svtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<svtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  svtkm::cont::DataSetBuilderUniform dataSetBuilder;
  svtkm::cont::DataSetFieldAdd dataSetField;

  svtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  dataSetField.AddPointField(ds, "vector", vectorField);

  return ds;
}

void TestStreamline()
{
  const svtkm::Id3 dims(5, 5, 5);
  const svtkm::Vec3f vecX(1, 0, 0);

  svtkm::cont::DataSet ds = CreateDataSet(dims, vecX);
  svtkm::cont::ArrayHandle<svtkm::Particle> seedArray;
  std::vector<svtkm::Particle> seeds(3);
  seeds[0] = svtkm::Particle(svtkm::Vec3f(.2f, 1.0f, .2f), 0);
  seeds[1] = svtkm::Particle(svtkm::Vec3f(.2f, 2.0f, .2f), 1);
  seeds[2] = svtkm::Particle(svtkm::Vec3f(.2f, 3.0f, .2f), 2);

  seedArray = svtkm::cont::make_ArrayHandle(seeds);

  svtkm::filter::Streamline streamline;

  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(20);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField("vector");
  auto output = streamline.Execute(ds);

  //Validate the result is correct.
  SVTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  svtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 63, "Wrong number of coordinates");

  svtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  SVTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
}

void TestPathline()
{
  const svtkm::Id3 dims(5, 5, 5);
  const svtkm::Vec3f vecX(1, 0, 0);
  const svtkm::Vec3f vecY(0, 1, 0);

  svtkm::cont::DataSet ds1 = CreateDataSet(dims, vecX);
  svtkm::cont::DataSet ds2 = CreateDataSet(dims, vecY);

  svtkm::cont::ArrayHandle<svtkm::Particle> seedArray;
  std::vector<svtkm::Particle> seeds(3);
  seeds[0] = svtkm::Particle(svtkm::Vec3f(.2f, 1.0f, .2f), 0);
  seeds[1] = svtkm::Particle(svtkm::Vec3f(.2f, 2.0f, .2f), 1);
  seeds[2] = svtkm::Particle(svtkm::Vec3f(.2f, 3.0f, .2f), 2);

  seedArray = svtkm::cont::make_ArrayHandle(seeds);

  svtkm::filter::Pathline pathline;

  pathline.SetPreviousTime(0.0f);
  pathline.SetNextTime(1.0f);
  pathline.SetNextDataSet(ds2);
  pathline.SetStepSize(static_cast<svtkm::FloatDefault>(0.05f));
  pathline.SetNumberOfSteps(20);
  pathline.SetSeeds(seedArray);

  pathline.SetActiveField("vector");
  auto output = pathline.Execute(ds1);

  //Validate the result is correct.
  svtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 63, "Wrong number of coordinates");

  svtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  SVTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
}

void TestStreamlineFilters()
{
  TestStreamline();
  TestPathline();
}
}

int UnitTestStreamlineFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStreamlineFilters, argc, argv);
}
