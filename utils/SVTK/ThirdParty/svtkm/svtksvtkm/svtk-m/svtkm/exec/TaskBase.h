//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_TaskBase_h
#define svtk_m_exec_TaskBase_h

#include <svtkm/Types.h>

#include <svtkm/exec/internal/ErrorMessageBuffer.h>

namespace svtkm
{
namespace exec
{

/// Base class for all classes that are used to marshal data from the invocation
/// parameters to the user worklets when invoked in the execution environment.
class TaskBase
{
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_TaskBase_h
