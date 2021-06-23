//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "ABCfield.h"
#include <iostream>
#include <vector>
#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/filter/Lagrangian.h>

using namespace std;

svtkm::cont::DataSet make3DRectilinearDataSet(double time)
{
  ABCfield field;

  double xmin, xmax, ymin, ymax, zmin, zmax;
  xmin = 0.0;
  ymin = 0.0;
  zmin = 0.0;

  xmax = 6.28;
  ymax = 6.28;
  zmax = 6.28;

  int dims[3] = { 16, 16, 16 };

  svtkm::cont::DataSetBuilderUniform dsb;

  double xdiff = (xmax - xmin) / (dims[0] - 1);
  double ydiff = (ymax - ymin) / (dims[1] - 1);
  double zdiff = (zmax - zmin) / (dims[2] - 1);

  svtkm::Id3 DIMS(dims[0], dims[1], dims[2]);
  svtkm::Vec3f_64 ORIGIN(0, 0, 0);
  svtkm::Vec3f_64 SPACING(xdiff, ydiff, zdiff);

  svtkm::cont::DataSet dataset = dsb.Create(DIMS, ORIGIN, SPACING);
  svtkm::cont::DataSetFieldAdd dsf;

  int numPoints = dims[0] * dims[1] * dims[2];

  svtkm::cont::ArrayHandle<svtkm::Vec3f_64> velocityField;
  velocityField.Allocate(numPoints);

  int count = 0;
  for (int i = 0; i < dims[0]; i++)
  {
    for (int j = 0; j < dims[1]; j++)
    {
      for (int k = 0; k < dims[2]; k++)
      {
        double vec[3];
        double loc[3] = { i * xdiff + xmin, j * ydiff + ymax, k * zdiff + zmin };
        field.calculateVelocity(loc, time, vec);
        velocityField.GetPortalControl().Set(count, svtkm::Vec3f_64(vec[0], vec[1], vec[2]));
        count++;
      }
    }
  }
  dsf.AddPointField(dataset, "velocity", velocityField);
  return dataset;
}


int main(int argc, char** argv)
{
  auto opts =
    svtkm::cont::InitializeOptions::DefaultAnyDevice | svtkm::cont::InitializeOptions::Strict;
  svtkm::cont::Initialize(argc, argv, opts);

  svtkm::filter::Lagrangian lagrangianFilter;
  lagrangianFilter.SetResetParticles(true);
  svtkm::Float32 stepSize = 0.01f;
  lagrangianFilter.SetStepSize(stepSize);
  lagrangianFilter.SetWriteFrequency(10);
  for (int i = 0; i < 100; i++)
  {
    svtkm::cont::DataSet inputData = make3DRectilinearDataSet((double)i * stepSize);
    lagrangianFilter.SetActiveField("velocity");
    svtkm::cont::DataSet extractedBasisFlows = lagrangianFilter.Execute(inputData);
  }
  return 0;
}
