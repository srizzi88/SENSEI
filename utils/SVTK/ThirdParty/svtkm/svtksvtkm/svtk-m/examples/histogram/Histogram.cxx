//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

/*
 * This example demonstrates how one can write a filter that uses MPI
 * for hybrid-parallelism. The `svtkm::filter::Histogram` is another approach for
 * implementing the same that uses DIY. This example doesn't use DIY, instead
 * uses MPI calls directly.
 */

#include "HistogramMPI.h"

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/Initialize.h>

#include <svtkm/thirdparty/diy/diy.h>

#include <mpi.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace
{
template <typename T>
SVTKM_CONT svtkm::cont::ArrayHandle<T> CreateArray(T min, T max, svtkm::Id numVals)
{
  std::mt19937 gen;
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  svtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  std::generate(svtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                svtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}
}

int main(int argc, char* argv[])
{
  //parse out all svtk-m related command line options
  auto opts =
    svtkm::cont::InitializeOptions::DefaultAnyDevice | svtkm::cont::InitializeOptions::Strict;
  svtkm::cont::Initialize(argc, argv, opts);

  // setup MPI environment.
  MPI_Init(&argc, &argv);

  // tell SVTK-m the communicator to use.
  svtkm::cont::EnvironmentTracker::SetCommunicator(svtkmdiy::mpi::communicator(MPI_COMM_WORLD));

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 2)
  {
    if (rank == 0)
    {
      std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <num-bins>" << std::endl;
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  const svtkm::Id num_bins = static_cast<svtkm::Id>(std::atoi(argv[1]));
  const svtkm::Id numVals = 1024;

  svtkm::cont::PartitionedDataSet pds;
  svtkm::cont::DataSet ds;
  svtkm::cont::DataSetFieldAdd::AddPointField(ds, "pointvar", CreateArray(-1024, 1024, numVals));
  pds.AppendPartition(ds);

  example::HistogramMPI histogram;
  histogram.SetActiveField("pointvar");
  histogram.SetNumberOfBins(std::max<svtkm::Id>(1, num_bins));
  svtkm::cont::PartitionedDataSet result = histogram.Execute(pds);

  svtkm::cont::ArrayHandle<svtkm::Id> bins;
  result.GetPartition(0).GetField("histogram").GetData().CopyTo(bins);
  auto binPortal = bins.GetPortalConstControl();
  if (rank == 0)
  {
    // print histogram.
    std::cout << "Histogram (" << num_bins << ")" << std::endl;
    svtkm::Id count = 0;
    for (svtkm::Id cc = 0; cc < num_bins; ++cc)
    {
      std::cout << "  bin[" << cc << "] = " << binPortal.Get(cc) << std::endl;
      count += binPortal.Get(cc);
    }
    if (count != numVals * size)
    {
      std::cout << "ERROR: bins mismatched!" << std::endl;
      MPI_Finalize();
      return EXIT_FAILURE;
    }
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
