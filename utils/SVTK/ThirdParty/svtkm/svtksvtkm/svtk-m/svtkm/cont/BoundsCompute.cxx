//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/BoundsCompute.h>

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <numeric> // for std::accumulate

namespace svtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::DataSet& dataset, svtkm::Id coordinate_system_index)
{
  return dataset.GetNumberOfCoordinateSystems() > coordinate_system_index
    ? dataset.GetCoordinateSystem(coordinate_system_index).GetBounds()
    : svtkm::Bounds();
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::PartitionedDataSet& pds,
                           svtkm::Id coordinate_system_index)
{
  return std::accumulate(pds.begin(),
                         pds.end(),
                         svtkm::Bounds(),
                         [=](const svtkm::Bounds& val, const svtkm::cont::DataSet& partition) {
                           return val +
                             svtkm::cont::BoundsCompute(partition, coordinate_system_index);
                         });
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::DataSet& dataset, const std::string& name)
{
  try
  {
    return dataset.GetCoordinateSystem(name).GetBounds();
  }
  catch (svtkm::cont::ErrorBadValue&)
  {
    // missing coordinate_system_index, return empty bounds.
    return svtkm::Bounds();
  }
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::PartitionedDataSet& pds, const std::string& name)
{
  return std::accumulate(pds.begin(),
                         pds.end(),
                         svtkm::Bounds(),
                         [=](const svtkm::Bounds& val, const svtkm::cont::DataSet& partition) {
                           return val + svtkm::cont::BoundsCompute(partition, name);
                         });
}
}
}
