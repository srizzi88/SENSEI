//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_FieldRangeCompute_h
#define svtk_m_cont_FieldRangeCompute_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <svtkm/cont/FieldRangeCompute.hxx>

namespace svtkm
{
namespace cont
{
/// \brief Compute ranges for fields in a DataSet or PartitionedDataSet.
///
/// These methods to compute ranges for fields in a single dataset or a
/// partitioned dataset.
/// When using SVTK-m in a hybrid-parallel environment with distributed processing,
/// this class uses ranges for locally available data alone. Use FieldRangeGlobalCompute
/// to compute ranges globally across all ranks even in distributed mode.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY);

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  SVTKM_IS_LIST(TypeList);
  return svtkm::cont::detail::FieldRangeComputeImpl(dataset, name, assoc, TypeList());
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
///
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY);

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  SVTKM_IS_LIST(TypeList);
  return svtkm::cont::detail::FieldRangeComputeImpl(pds, name, assoc, TypeList());
}

//@}
}
} // namespace svtkm::cont

#endif
