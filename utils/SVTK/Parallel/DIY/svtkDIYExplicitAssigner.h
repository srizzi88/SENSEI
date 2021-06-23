/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIYExplicitAssigner.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkDIYExplicitAssigner
 * @brief assigner for use with DIY
 *
 * svtkDIYExplicitAssigner is a diy::StaticAssigner specialization that be used
 * where the block assignment is not strictly round-robin or contiguous which
 * assumes blocks equally split among ranks. This supports the case where each
 * rank has arbitrary number of blocks per rank. The constructor is provided the
 * mpi communicator and the number of local blocks. It performs parallel
 * communication to exchange information about blocks with all participating
 * ranks.
 *
 * svtkDIYExplicitAssigner also supports ability to pad each rank such that the
 * total number of blocks across all ranks is a power of two.
 */

#ifndef svtkDIYExplicitAssigner_h
#define svtkDIYExplicitAssigner_h

#include "svtkObject.h"
#include "svtkParallelDIYModule.h" // for export macros
// clang-format off
#include "svtk_diy2.h"
#include SVTK_DIY2(diy/mpi.hpp)
#include SVTK_DIY2(diy/assigner.hpp)
// clang-format on

#ifdef _WIN32
#pragma warning(disable : 4275) /* non dll-interface class `diy::StaticAssigner` used as base for  \
                                   dll-interface class */
#endif

class SVTKPARALLELDIY_EXPORT svtkDIYExplicitAssigner : public diy::StaticAssigner
{
public:
  svtkDIYExplicitAssigner(
    diy::mpi::communicator comm, int local_blocks, bool force_power_of_two = false);

  int rank(int gid) const override;
  void local_gids(int rank, std::vector<int>& gids) const override;

private:
  std::vector<int> IScanBlockCounts;
};

#endif
// SVTK-HeaderTest-Exclude: svtkDIYExplicitAssigner.h
