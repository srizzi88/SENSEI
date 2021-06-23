//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <cmath>
#include <string>
#include <vector>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/Initialize.h>

#include <svtkm/io/reader/SVTKDataSetReader.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/filter/LagrangianStructures.h>

int main(int argc, char** argv)
{
  svtkm::cont::Initialize(argc, argv);

  if (argc < 3)
  {
    std::cout << "Usage : flte <input dataset> <vector field name>" << std::endl;
  }
  std::string datasetName(argv[1]);
  std::string variableName(argv[2]);

  std::cout << "Reading input dataset" << std::endl;
  svtkm::cont::DataSet input;
  svtkm::io::reader::SVTKDataSetReader reader(datasetName);
  input = reader.ReadDataSet();
  std::cout << "Read input dataset" << std::endl;

  svtkm::filter::LagrangianStructures lcsFilter;
  lcsFilter.SetStepSize(0.025f);
  lcsFilter.SetNumberOfSteps(500);
  lcsFilter.SetAdvectionTime(0.025f * 500);
  lcsFilter.SetOutputFieldName("gradient");
  lcsFilter.SetActiveField(variableName);

  svtkm::cont::DataSet output = lcsFilter.Execute(input);
  svtkm::io::writer::SVTKDataSetWriter writer("out.svtk");
  writer.WriteDataSet(output);
  std::cout << "Written output dataset" << std::endl;

  return 0;
}
