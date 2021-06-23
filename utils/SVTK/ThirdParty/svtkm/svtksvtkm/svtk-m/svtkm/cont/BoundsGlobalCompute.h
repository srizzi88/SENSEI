//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_BoundsGlobalCompute_h
#define svtk_m_cont_BoundsGlobalCompute_h

#include <svtkm/Bounds.h>
#include <svtkm/cont/svtkm_cont_export.h>

namespace svtkm
{
namespace cont
{

class DataSet;
class PartitionedDataSet;

//@{
/// \brief Functions to compute bounds for a single dataset or partitioned
/// dataset globally
///
/// These are utility functions that compute bounds for a single dataset or
/// partitioned dataset globally i.e. across all ranks when operating in a
/// distributed environment. When SVTK-m not operating in an distributed
/// environment, these behave same as `svtkm::cont::BoundsCompute`.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for PartitionedDataSet, partitions without the
/// chosen CoordinateSystem are skipped.
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::DataSet& dataset,
                                 svtkm::Id coordinate_system_index = 0);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::PartitionedDataSet& pds,
                                 svtkm::Id coordinate_system_index = 0);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::DataSet& dataset,
                                 const std::string& coordinate_system_name);

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::Bounds BoundsGlobalCompute(const svtkm::cont::PartitionedDataSet& pds,
                                 const std::string& coordinate_system_name);
//@}
}
}
#endif
