//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/FieldRangeCompute.h>
#include <svtkm/cont/FieldRangeCompute.hxx>

#include <svtkm/cont/Algorithm.h>

namespace svtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(const svtkm::cont::DataSet& dataset,
                                                       const std::string& name,
                                                       svtkm::cont::Field::Association assoc)
{
  return svtkm::cont::detail::FieldRangeComputeImpl(dataset, name, assoc, SVTKM_DEFAULT_TYPE_LIST());
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeCompute(const svtkm::cont::PartitionedDataSet& pds,
                                                       const std::string& name,
                                                       svtkm::cont::Field::Association assoc)
{
  return svtkm::cont::detail::FieldRangeComputeImpl(pds, name, assoc, SVTKM_DEFAULT_TYPE_LIST());
}
}
} // namespace svtkm::cont
