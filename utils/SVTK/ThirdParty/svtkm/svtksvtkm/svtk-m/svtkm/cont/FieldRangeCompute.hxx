//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_FieldRangeCompute_hxx
#define svtk_m_cont_FieldRangeCompute_hxx

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <svtkm/Range.h>

#include <numeric> // for std::accumulate

namespace svtkm
{
namespace cont
{
namespace detail
{

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeComputeImpl(
  const svtkm::cont::DataSet& dataset,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  svtkm::cont::Field field;
  try
  {
    field = dataset.GetField(name, assoc);
  }
  catch (svtkm::cont::ErrorBadValue&)
  {
    // field missing, return empty range.
    return svtkm::cont::ArrayHandle<svtkm::Range>();
  }

  return field.GetRange(TypeList());
}

template <typename TypeList>
SVTKM_CONT svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeComputeImpl(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc,
  TypeList)
{
  std::vector<svtkm::Range> result_vector = std::accumulate(
    pds.begin(),
    pds.end(),
    std::vector<svtkm::Range>(),
    [&](const std::vector<svtkm::Range>& accumulated_value, const svtkm::cont::DataSet& dataset) {
      svtkm::cont::ArrayHandle<svtkm::Range> partition_range =
        svtkm::cont::detail::FieldRangeComputeImpl(dataset, name, assoc, TypeList());

      std::vector<svtkm::Range> result = accumulated_value;

      // if the current partition has more components than we have seen so far,
      // resize the result to fit all components.
      result.resize(
        std::max(result.size(), static_cast<size_t>(partition_range.GetNumberOfValues())));

      auto portal = partition_range.GetPortalConstControl();
      std::transform(svtkm::cont::ArrayPortalToIteratorBegin(portal),
                     svtkm::cont::ArrayPortalToIteratorEnd(portal),
                     result.begin(),
                     result.begin(),
                     std::plus<svtkm::Range>());
      return result;
    });

  return svtkm::cont::make_ArrayHandle(result_vector, svtkm::CopyFlag::On);
}
}
}
} //  namespace svtkm::cont::detail

#endif
