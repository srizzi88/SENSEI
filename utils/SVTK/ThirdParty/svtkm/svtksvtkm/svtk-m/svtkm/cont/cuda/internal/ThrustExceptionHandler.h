//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_interal_ThrustExecptionHandler_h
#define svtk_m_cont_cuda_interal_ThrustExecptionHandler_h

#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorExecution.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/exec/cuda/internal/ThrustPatches.h>
SVTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system_error.h>
SVTKM_THIRDPARTY_POST_INCLUDE

namespace svtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

static inline void throwAsSVTKmException()
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (std::bad_alloc& error)
  {
    throw svtkm::cont::ErrorBadAllocation(error.what());
  }
  catch (thrust::system_error& error)
  {
    throw svtkm::cont::ErrorExecution(error.what());
  }
}
}
}
}
}

#endif //svtk_m_cont_cuda_interal_ThrustExecptionHandler_h
