//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_FieldRangeGlobalCompute_h
#define svtk_m_cont_FieldRangeGlobalCompute_h

#include <svtkm/cont/FieldRangeCompute.h>

#include <svtkm/cont/FieldRangeGlobalCompute.hxx>

namespace svtkm
{
namespace cont
{
/// \brief utility functions to compute global ranges for dataset fields.
///
/// These functions compute global ranges for fields in a single DataSet or a
/// PartitionedDataSet.
/// In non-distributed environments, this is exactly same as `FieldRangeCompute`. In
/// distributed environments, however, the range is computed locally on each rank
/// and then a reduce-all collective is performed to reduces the ranges on all ranks.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY);

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  SVTKM_IS_LIST(TypeList);
  return detail::FieldRangeGlobalComputeImpl(dataset, name, assoc, TypeList());
}

//@}

//{@
/// Returns the range for a field from a PartitionedDataSet. If the field is
/// not present on any of the partitions, an empty ArrayHandle will be
/// returned. If the field is present on some partitions, but not all, those
/// partitions without the field are skipped.
///
/// The returned array handle will have as many values as the maximum number of
/// components for the selected field across all partitions.
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY);

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  SVTKM_IS_LIST(TypeList);
  return detail::FieldRangeGlobalComputeImpl(pds, name, assoc, TypeList());
}
//@}
}
} // namespace svtkm::cont

#endif
