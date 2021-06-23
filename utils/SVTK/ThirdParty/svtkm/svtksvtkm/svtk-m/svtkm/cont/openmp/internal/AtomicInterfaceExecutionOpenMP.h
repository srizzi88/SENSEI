//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h
#define svtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h

#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <svtkm/cont/internal/AtomicInterfaceControl.h>
#include <svtkm/cont/internal/AtomicInterfaceExecution.h>

#include <svtkm/Types.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <>
class AtomicInterfaceExecution<DeviceAdapterTagOpenMP> : public AtomicInterfaceControl
{
};
}
}
} // end namespace svtkm::cont::internal

#endif // svtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h
