//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/filter/Streamline.h>
#include <svtkm/io/reader/SVTKDataSetReader.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

// Example computing streamlines.
// An example vector field is available in the svtk-m data directory: magField.svtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.01
//
// Particle_Advection <path-to-data-dir>/magField.svtk vec 200 50 0.01 output.svtk
//

int main(int argc, char** argv)
{
  auto opts = svtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = svtkm::cont::Initialize(argc, argv, opts);

  if (argc < 8)
  {
    std::cerr << "Usage: " << argv[0]
              << "dataFile varName numSeeds numSteps stepSize outputFile [options]" << std::endl;
    std::cerr << "where options are: " << std::endl << config.Usage << std::endl;
    return -1;
  }

  std::string dataFile = argv[1];
  std::string varName = argv[2];
  svtkm::Id numSeeds = std::stoi(argv[3]);
  svtkm::Id numSteps = std::stoi(argv[4]);
  svtkm::FloatDefault stepSize = std::stof(argv[5]);
  std::string outputFile = argv[6];

  svtkm::cont::DataSet ds;

  if (dataFile.find(".svtk") != std::string::npos)
  {
    svtkm::io::reader::SVTKDataSetReader rdr(dataFile);
    ds = rdr.ReadDataSet();
  }
  else
  {
    std::cerr << "Unsupported data file: " << dataFile << std::endl;
    return -1;
  }

  //create seeds randomly placed withing the bounding box of the data.
  svtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();
  std::vector<svtkm::Particle> seeds;

  for (svtkm::Id i = 0; i < numSeeds; i++)
  {
    svtkm::Particle p;
    svtkm::FloatDefault rx = (svtkm::FloatDefault)rand() / (svtkm::FloatDefault)RAND_MAX;
    svtkm::FloatDefault ry = (svtkm::FloatDefault)rand() / (svtkm::FloatDefault)RAND_MAX;
    svtkm::FloatDefault rz = (svtkm::FloatDefault)rand() / (svtkm::FloatDefault)RAND_MAX;
    p.Pos[0] = static_cast<svtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
    p.Pos[1] = static_cast<svtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
    p.Pos[2] = static_cast<svtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
    p.ID = i;
    seeds.push_back(p);
  }
  auto seedArray = svtkm::cont::make_ArrayHandle(seeds);

  //compute streamlines
  svtkm::filter::Streamline streamline;

  streamline.SetStepSize(stepSize);
  streamline.SetNumberOfSteps(numSteps);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField(varName);
  auto output = streamline.Execute(ds);

  svtkm::io::writer::SVTKDataSetWriter wrt(outputFile);
  wrt.WriteDataSet(output);

  return 0;
}
