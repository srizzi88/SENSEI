//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_tbb_internal_TaskTiling_h
#define svtk_m_exec_tbb_internal_TaskTiling_h

#include <svtkm/exec/serial/internal/TaskTiling.h>

namespace svtkm
{
namespace exec
{
namespace tbb
{
namespace internal
{

using TaskTiling1D = svtkm::exec::serial::internal::TaskTiling1D;
using TaskTiling3D = svtkm::exec::serial::internal::TaskTiling3D;
}
}
}
} // namespace svtkm::exec::tbb::internal

#endif //svtk_m_exec_tbb_internal_TaskTiling_h
