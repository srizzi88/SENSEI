//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/Initialize.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>

#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/filter/ClipWithField.h>

int main(int argc, char* argv[])
{
  svtkm::cont::Initialize(argc, argv, svtkm::cont::InitializeOptions::Strict);

  svtkm::cont::DataSet input = svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetCowNose();

  svtkm::filter::ClipWithField clipFilter;
  clipFilter.SetActiveField("pointvar");
  clipFilter.SetClipValue(20.0);
  svtkm::cont::DataSet output = clipFilter.Execute(input);

  svtkm::io::writer::SVTKDataSetWriter writer("out_data.svtk");
  writer.WriteDataSet(output);

  return 0;
}
