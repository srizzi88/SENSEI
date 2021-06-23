//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_internal_RunTriangulator_h
#define svtk_m_rendering_internal_RunTriangulator_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

namespace svtkm
{
namespace rendering
{
namespace internal
{

/// This is a wrapper around the Triangulator worklet so that the
/// implementation of the triangulator only gets compiled once. This function
/// really is a stop-gap. Eventually, the Triangulator should be moved to
/// filters, and filters should be compiled in a library (for the same reason).
///
SVTKM_RENDERING_EXPORT
void RunTriangulator(const svtkm::cont::DynamicCellSet& cellSet,
                     svtkm::cont::ArrayHandle<svtkm::Id4>& indices,
                     svtkm::Id& numberOfTriangles);
}
}
} // namespace svtkm::rendering::internal

#endif //svtk_m_rendering_internal_RunTriangulator_h
