//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_FieldRangeGlobalCompute_hxx
#define svtk_m_cont_FieldRangeGlobalCompute_hxx

namespace svtkm
{
namespace cont
{
namespace detail
{

SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> MergeRangesGlobal(
  const svtkm::cont::ArrayHandle<svtkm::Range>& range);

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalComputeImpl(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  auto lrange = svtkm::cont::FieldRangeCompute(dataset, name, assoc, TypeList());
  return svtkm::cont::detail::MergeRangesGlobal(lrange);
}

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalComputeImpl(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  auto lrange = svtkm::cont::FieldRangeCompute(pds, name, assoc, TypeList());
  return svtkm::cont::detail::MergeRangesGlobal(lrange);
}
}
}
}

#endif
