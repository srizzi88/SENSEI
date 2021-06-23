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
#include <svtkm/filter/StreamSurface.h>

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

void TestStreamSurface()
{
  std::cout << "Testing Stream Surface Filter" << std::endl;

  const svtkm::Id3 dims(5, 5, 5);
  const svtkm::Vec3f vecX(1, 0, 0);

  svtkm::cont::DataSet ds = CreateDataSet(dims, vecX);
  svtkm::cont::ArrayHandle<svtkm::Particle> seedArray;
  std::vector<svtkm::Particle> seeds(4);
  seeds[0] = svtkm::Particle(svtkm::Vec3f(.1f, 1.0f, .2f), 0);
  seeds[1] = svtkm::Particle(svtkm::Vec3f(.1f, 2.0f, .1f), 1);
  seeds[2] = svtkm::Particle(svtkm::Vec3f(.1f, 3.0f, .3f), 2);
  seeds[3] = svtkm::Particle(svtkm::Vec3f(.1f, 3.5f, .2f), 3);

  seedArray = svtkm::cont::make_ArrayHandle(seeds);

  svtkm::filter::StreamSurface streamSrf;

  streamSrf.SetStepSize(0.1f);
  streamSrf.SetNumberOfSteps(20);
  streamSrf.SetSeeds(seedArray);
  streamSrf.SetActiveField("vector");

  auto output = streamSrf.Execute(ds);

  //Validate the result is correct.
  SVTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  svtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 84, "Wrong number of coordinates");

  svtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  SVTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 120, "Wrong number of cells");
}
}

int UnitTestStreamSurfaceFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStreamSurface, argc, argv);
}
