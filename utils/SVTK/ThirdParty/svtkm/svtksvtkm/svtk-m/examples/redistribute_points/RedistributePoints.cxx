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
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/Initialize.h>

#include <svtkm/io/reader/SVTKDataSetReader.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/thirdparty/diy/diy.h>

#include "RedistributePoints.h"

#include <sstream>
using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  // Process svtk-m general args
  auto opts = svtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = svtkm::cont::Initialize(argc, argv, opts);

  svtkmdiy::mpi::environment env(argc, argv);
  auto comm = svtkmdiy::mpi::communicator(MPI_COMM_WORLD);
  svtkm::cont::EnvironmentTracker::SetCommunicator(comm);

  if (argc != 3)
  {
    cout << "Usage: " << endl
         << "$ " << argv[0] << " [options] <input-svtk-file> <output-file-prefix>" << endl;
    cout << config.Usage << endl;
    return EXIT_FAILURE;
  }

  svtkm::cont::DataSet input;
  if (comm.rank() == 0)
  {
    svtkm::io::reader::SVTKDataSetReader reader(argv[1]);
    input = reader.ReadDataSet();
  }

  example::RedistributePoints redistributor;
  auto output = redistributor.Execute(input);

  std::ostringstream str;
  str << argv[2] << "-" << comm.rank() << ".svtk";

  svtkm::io::writer::SVTKDataSetWriter writer(str.str());
  writer.WriteDataSet(output);
  return EXIT_SUCCESS;
}
