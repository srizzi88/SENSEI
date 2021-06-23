//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cstdlib>
#include <string>
#include <vector>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/filter/Pathline.h>

#include <svtkm/io/reader/SVTKDataSetReader.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = svtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = svtkm::cont::Initialize(argc, argv, opts);

  // Sample data to use this example can be found in the data directory of the
  // SVTK-m repo in the location temporal_datasets.
  // These example with these datasets can be used for this example as :
  // ./Temporal_Advection DoubleGyre_0.svtk 0.0 DoubleGyre_5.svtk 5.0
  //                      velocity 500 0.025 pathlines.svtk
  std::cout
    << "Parameters : [options] slice1 time1 slice2 time2 field num_steps step_size output\n"
    << "slice1 : Time slice 1, sample data in svtk-m/data/temporal_datasets/Double_Gyre0.svtk\n"
    << "time1 : simulation time for slice 1, for sample data use 0.0\n"
    << "slice2 : Time slice 2, sample data in svtk-m/data/temporal_datasets/Double_Gyre5.svtk\n"
    << "time2 : simulation time for slice 2, for sample data use 5.0\n"
    << "field : active velocity field in the data set, for sample data use 'velocity'\n"
    << "num_steps : maximum number of steps for advection, for sample data use 500\n"
    << "step_size : the size of a single step during advection, for sample data use 0.025\n"
    << "output : the name of the output file" << std::endl;

  if (argc < 8)
  {
    std::cout << "Wrong number of parameters provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string fieldName, datasetName1, datasetName2, outputName;
  svtkm::FloatDefault time1, time2;

  svtkm::Id numSteps;
  svtkm::Float32 stepSize;

  datasetName1 = std::string(argv[1]);
  time1 = static_cast<svtkm::FloatDefault>(atof(argv[2]));
  datasetName2 = std::string(argv[3]);
  time2 = static_cast<svtkm::FloatDefault>(atof(argv[4]));
  fieldName = std::string(argv[5]);
  numSteps = atoi(argv[6]);
  stepSize = static_cast<svtkm::Float32>(atof(argv[7]));
  outputName = std::string(argv[8]);

  svtkm::io::reader::SVTKDataSetReader reader1(datasetName1);
  svtkm::cont::DataSet ds1 = reader1.ReadDataSet();

  svtkm::io::reader::SVTKDataSetReader reader2(datasetName2);
  svtkm::cont::DataSet ds2 = reader2.ReadDataSet();

  // Use the coordinate system as seeds for performing advection
  svtkm::cont::ArrayHandle<svtkm::Vec3f> pts;
  svtkm::cont::ArrayCopy(ds1.GetCoordinateSystem().GetData(), pts);
  svtkm::cont::ArrayHandle<svtkm::Particle> seeds;

  svtkm::Id numPts = pts.GetNumberOfValues();
  seeds.Allocate(numPts);
  auto ptsPortal = pts.GetPortalConstControl();
  auto seedPortal = seeds.GetPortalConstControl();
  for (svtkm::Id i = 0; i < numPts; i++)
  {
    svtkm::Particle p;
    p.Pos = ptsPortal.Get(i);
    p.ID = i;
    seedPortal.Set(i, p);
  }

  // Instantiate the filter by providing necessary parameters.
  // Necessary parameters are :
  svtkm::filter::Pathline pathlineFilter;
  pathlineFilter.SetActiveField(fieldName);
  // 1. The current and next time slice. The current time slice is passed
  //    through the parameter to the Execute method.
  pathlineFilter.SetNextDataSet(ds2);
  // 2. The current and next times, these times will be used to interpolate
  //    the velocities for particle positions in space and time.
  pathlineFilter.SetPreviousTime(time1);
  pathlineFilter.SetNextTime(time2);
  // 3. Maximum number of steps the particle is allowed to take until termination.
  pathlineFilter.SetNumberOfSteps(numSteps);
  // 4. Length for each step.
  pathlineFilter.SetStepSize(stepSize);
  // 5. Seeds for advection.
  pathlineFilter.SetSeeds(seeds);

  svtkm::cont::DataSet output = pathlineFilter.Execute(ds1);

  // The way to verify if the code produces correct streamlines
  // is to do a visual test by using VisIt/ParaView to visualize
  // the file written by this method.
  svtkm::io::writer::SVTKDataSetWriter writer(outputName);
  writer.WriteDataSet(output);
  return 0;
}
