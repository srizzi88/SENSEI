//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/internal/RunTriangulator.h>

#include <svtkm/cont/TryExecute.h>
#include <svtkm/rendering/Triangulator.h>

namespace svtkm
{
namespace rendering
{
namespace internal
{

void RunTriangulator(const svtkm::cont::DynamicCellSet& cellSet,
                     svtkm::cont::ArrayHandle<svtkm::Id4>& indices,
                     svtkm::Id& numberOfTriangles)
{
  svtkm::rendering::Triangulator triangulator;
  triangulator.Run(cellSet, indices, numberOfTriangles);
}
}
}
} // namespace svtkm::rendering::internal
