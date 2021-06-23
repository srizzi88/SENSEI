//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_CellLocator_h
#define svtk_m_exec_CellLocator_h

#include <svtkm/Types.h>
#include <svtkm/VirtualObjectBase.h>
#include <svtkm/exec/FunctorBase.h>

namespace svtkm
{
namespace exec
{

class SVTKM_ALWAYS_EXPORT CellLocator : public svtkm::VirtualObjectBase
{
public:
  SVTKM_EXEC_CONT virtual ~CellLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC
  virtual void FindCell(const svtkm::Vec3f& point,
                        svtkm::Id& cellId,
                        svtkm::Vec3f& parametric,
                        const svtkm::exec::FunctorBase& worklet) const = 0;
};

} // namespace exec
} // namespace svtkm

#endif // svtk_m_exec_CellLocator_h
