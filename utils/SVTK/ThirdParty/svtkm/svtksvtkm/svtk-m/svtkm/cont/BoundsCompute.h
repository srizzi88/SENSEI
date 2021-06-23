//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_BoundsCompute_h
#define svtk_m_cont_BoundsCompute_h

#include <svtkm/Bounds.h>
#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace cont
{

class DataSet;
class PartitionedDataSet;

//@{
/// \brief Functions to compute bounds for a single dataSset or partition dataset
///
/// These are utility functions that compute bounds for a single dataset or
/// partitioned dataset. When SVTK-m is operating in an distributed environment,
/// these are bounds on the local process. To get global bounds across all
/// ranks, use `svtkm::cont::BoundsGlobalCompute` instead.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for PartitionedDataSet, partitions without the
/// chosen CoordinateSystem are skipped.
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::DataSet& dataset,
                           svtkm::Id coordinate_system_index = 0);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::PartitionedDataSet& pds,
                           svtkm::Id coordinate_system_index = 0);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::DataSet& dataset,
                           const std::string& coordinate_system_name);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsCompute(const svtkm::cont::PartitionedDataSet& pds,
                           const std::string& coordinate_system_name);
//@}
}
}

#endif
