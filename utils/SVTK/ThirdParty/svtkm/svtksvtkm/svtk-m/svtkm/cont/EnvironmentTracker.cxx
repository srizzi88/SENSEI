//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/EnvironmentTracker.h>

#include <svtkm/thirdparty/diy/diy.h>

namespace svtkm
{
namespace cont
{
namespace internal
{
static svtkmdiy::mpi::communicator GlobalCommuncator(MPI_COMM_NULL);
}

void EnvironmentTracker::SetCommunicator(const svtkmdiy::mpi::communicator& comm)
{
  svtkm::cont::internal::GlobalCommuncator = comm;
}

const svtkmdiy::mpi::communicator& EnvironmentTracker::GetCommunicator()
{
#ifndef SVTKM_DIY_NO_MPI
  int flag;
  MPI_Initialized(&flag);
  if (!flag)
  {
    int argc = 0;
    char** argv = nullptr;
    MPI_Init(&argc, &argv);
    internal::GlobalCommuncator = svtkmdiy::mpi::communicator(MPI_COMM_WORLD);
  }
#endif
  return svtkm::cont::internal::GlobalCommuncator;
}
} // namespace svtkm::cont
} // namespace svtkm
