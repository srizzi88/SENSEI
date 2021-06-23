//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <iostream>
#include <svtkm/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/Lagrangian.h>

svtkm::cont::DataSet MakeTestUniformDataSet()
{
  svtkm::Float64 xmin, xmax, ymin, ymax, zmin, zmax;
  xmin = 0.0;
  ymin = 0.0;
  zmin = 0.0;

  xmax = 10.0;
  ymax = 10.0;
  zmax = 10.0;

  const svtkm::Id3 DIMS(16, 16, 16);

  svtkm::cont::DataSetBuilderUniform dsb;

  svtkm::Float64 xdiff = (xmax - xmin) / (static_cast<svtkm::Float64>(DIMS[0] - 1));
  svtkm::Float64 ydiff = (ymax - ymin) / (static_cast<svtkm::Float64>(DIMS[1] - 1));
  svtkm::Float64 zdiff = (zmax - zmin) / (static_cast<svtkm::Float64>(DIMS[2] - 1));

  svtkm::Vec3f_64 ORIGIN(0, 0, 0);
  svtkm::Vec3f_64 SPACING(xdiff, ydiff, zdiff);

  svtkm::cont::DataSet dataset = dsb.Create(DIMS, ORIGIN, SPACING);
  svtkm::cont::DataSetFieldAdd dsf;

  svtkm::Id numPoints = DIMS[0] * DIMS[1] * DIMS[2];

  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> velocityField;
  velocityField.Allocate(numPoints);

  svtkm::Id count = 0;
  for (svtkm::Id i = 0; i < DIMS[0]; i++)
  {
    for (svtkm::Id j = 0; j < DIMS[1]; j++)
    {
      for (svtkm::Id k = 0; k < DIMS[2]; k++)
      {
        velocityField.GetPortalControl().Set(count, svtkm::Vec3f_64(0.1, 0.1, 0.1));
        count++;
      }
    }
  }
  dsf.AddPointField(dataset, "velocity", velocityField);
  return dataset;
}

void TestLagrangianFilterMultiStepInterval()
{
  std::cout << "Test: Lagrangian Analysis - Uniform Dataset - Write Interval > 1" << std::endl;
  svtkm::Id maxCycles = 10;
  svtkm::Id write_interval = 5;
  svtkm::filter::Lagrangian lagrangianFilter2;
  lagrangianFilter2.SetResetParticles(true);
  lagrangianFilter2.SetStepSize(0.1f);
  lagrangianFilter2.SetWriteFrequency(write_interval);
  for (svtkm::Id i = 1; i <= maxCycles; i++)
  {
    svtkm::cont::DataSet input = MakeTestUniformDataSet();
    lagrangianFilter2.SetActiveField("velocity");
    svtkm::cont::DataSet extractedBasisFlows = lagrangianFilter2.Execute(input);
    if (i % write_interval == 0)
    {
      SVTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset.");
      SVTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCells() == 3375,
                       "Wrong number of basis flows extracted.");
    }
    else
    {
      SVTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCells() == 0,
                       "Output dataset should have no cells.");
      SVTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 0,
                       "Wrong number of coordinate systems in the output dataset.");
    }
  }
}

void TestLagrangian()
{
  TestLagrangianFilterMultiStepInterval();
}

int UnitTestLagrangianFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestLagrangian, argc, argv);
}
