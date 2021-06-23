//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/BoundsGlobalCompute.h>

#include <svtkm/cont/BoundsCompute.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/FieldRangeGlobalCompute.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <numeric> // for std::accumulate

namespace svtkm
{
namespace cont
{

namespace detail
{
SVTKM_CONT
svtkm::Bounds MergeBoundsGlobal(const svtkm::Bounds& local)
{
  svtkm::cont::ArrayHandle<svtkm::Range> ranges;
  ranges.Allocate(3);
  ranges.GetPortalControl().Set(0, local.X);
  ranges.GetPortalControl().Set(1, local.Y);
  ranges.GetPortalControl().Set(2, local.Z);

  ranges = svtkm::cont::detail::MergeRangesGlobal(ranges);
  auto portal = ranges.GetPortalConstControl();
  return svtkm::Bounds(portal.Get(0), portal.Get(1), portal.Get(2));
}
}


//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::DataSet& dataset,
                                 svtkm::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(svtkm::cont::BoundsCompute(dataset, coordinate_system_index));
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::PartitionedDataSet& pds,
                                 svtkm::Id coordinate_system_index)
{
  return detail::MergeBoundsGlobal(svtkm::cont::BoundsCompute(pds, coordinate_system_index));
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::DataSet& dataset, const std::string& name)
{
  return detail::MergeBoundsGlobal(svtkm::cont::BoundsCompute(dataset, name));
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::PartitionedDataSet& pds, const std::string& name)
{
  return detail::MergeBoundsGlobal(svtkm::cont::BoundsCompute(pds, name));
}
}
}
