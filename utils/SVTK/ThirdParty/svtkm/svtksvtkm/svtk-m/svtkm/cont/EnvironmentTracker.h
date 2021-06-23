//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_EnvironmentTracker_h
#define svtk_m_cont_EnvironmentTracker_h

#include <svtkm/Types.h>
#include <svtkm/cont/svtkm_cont_export.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/thirdparty/diy/diy.h>

namespace svtkm
{
namespace cont
{

/// \brief Maintain MPI controller, if any, for distributed operation.
///
/// `EnvironmentTracker` is a class that provides static API to track the global
/// MPI controller to use for operating in a distributed environment.
class SVTKM_CONT_EXPORT EnvironmentTracker
{
public:
  SVTKM_CONT
  static void SetCommunicator(const svtkmdiy::mpi::communicator& comm);

  SVTKM_CONT
  static const svtkmdiy::mpi::communicator& GetCommunicator();
};
}
}


#endif // svtk_m_cont_EnvironmentTracker_h
