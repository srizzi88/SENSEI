//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_PointLocator_h
#define svtk_m_exec_PointLocator_h

#include <svtkm/VirtualObjectBase.h>

namespace svtkm
{
namespace exec
{

class SVTKM_ALWAYS_EXPORT PointLocator : public svtkm::VirtualObjectBase
{
public:
  SVTKM_EXEC_CONT virtual ~PointLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC virtual void FindNearestNeighbor(const svtkm::Vec3f& queryPoint,
                                             svtkm::Id& pointId,
                                             svtkm::FloatDefault& distanceSquared) const = 0;
};
}
}
#endif // svtk_m_exec_PointLocator_h
