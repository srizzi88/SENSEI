//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/FieldHistogram.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/AssignerPartitionedDataSet.h>
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/FieldRangeGlobalCompute.h>

#include <svtkm/thirdparty/diy/diy.h>

namespace example
{
namespace detail
{

class DistributedHistogram
{
  std::vector<svtkm::cont::ArrayHandle<svtkm::Id>> LocalBlocks;

public:
  DistributedHistogram(svtkm::Id numLocalBlocks)
    : LocalBlocks(static_cast<size_t>(numLocalBlocks))
  {
  }

  void SetLocalHistogram(svtkm::Id index, const svtkm::cont::ArrayHandle<svtkm::Id>& bins)
  {
    this->LocalBlocks[static_cast<size_t>(index)] = bins;
  }

  void SetLocalHistogram(svtkm::Id index, const svtkm::cont::Field& field)
  {
    this->SetLocalHistogram(index, field.GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Id>>());
  }

  svtkm::cont::ArrayHandle<svtkm::Id> ReduceAll(const svtkm::Id numBins) const
  {
    const svtkm::Id numLocalBlocks = static_cast<svtkm::Id>(this->LocalBlocks.size());
    auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? svtkm::cont::ArrayHandle<svtkm::Id>() : this->LocalBlocks[0];
    }


    // reduce local bins first.
    svtkm::cont::ArrayHandle<svtkm::Id> local;
    local.Allocate(numBins);
    std::fill(svtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalControl()),
              svtkm::cont::ArrayPortalToIteratorEnd(local.GetPortalControl()),
              static_cast<svtkm::Id>(0));
    for (const auto& lbins : this->LocalBlocks)
    {
      svtkm::cont::Algorithm::Transform(local, lbins, local, svtkm::Add());
    }

    // now reduce across ranks using MPI.

    // converting to std::vector
    std::vector<svtkm::Id> send_buf(static_cast<std::size_t>(numBins));
    std::copy(svtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalConstControl()),
              svtkm::cont::ArrayPortalToIteratorEnd(local.GetPortalConstControl()),
              send_buf.begin());

    std::vector<svtkm::Id> recv_buf(static_cast<std::size_t>(numBins));
    MPI_Reduce(&send_buf[0],
               &recv_buf[0],
               static_cast<int>(numBins),
               sizeof(svtkm::Id) == 4 ? MPI_INT : MPI_LONG,
               MPI_SUM,
               0,
               comm);

    if (comm.rank() == 0)
    {
      local.Allocate(numBins);
      std::copy(recv_buf.begin(),
                recv_buf.end(),
                svtkm::cont::ArrayPortalToIteratorBegin(local.GetPortalControl()));
      return local;
    }
    return svtkm::cont::ArrayHandle<svtkm::Id>();
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline SVTKM_CONT HistogramMPI::HistogramMPI()
  : NumberOfBins(10)
  , BinDelta(0)
  , ComputedRange()
  , Range()
{
  this->SetOutputFieldName("histogram");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet HistogramMPI::DoExecute(
  const svtkm::cont::DataSet&,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata&,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  svtkm::cont::ArrayHandle<svtkm::Id> binArray;
  T delta;

  svtkm::worklet::FieldHistogram worklet;
  if (this->ComputedRange.IsNonEmpty())
  {
    worklet.Run(field,
                this->NumberOfBins,
                static_cast<T>(this->ComputedRange.Min),
                static_cast<T>(this->ComputedRange.Max),
                delta,
                binArray);
  }
  else
  {
    worklet.Run(field, this->NumberOfBins, this->ComputedRange, delta, binArray);
  }

  this->BinDelta = static_cast<svtkm::Float64>(delta);
  svtkm::cont::DataSet output;
  svtkm::cont::Field rfield(
    this->GetOutputFieldName(), svtkm::cont::Field::Association::WHOLE_MESH, binArray);
  output.AddField(rfield);
  return output;
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT void HistogramMPI::PreExecute(const svtkm::cont::PartitionedDataSet& input,
                                               const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = svtkm::cont::FieldRangeGlobalCompute(
      input, this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    if (handle.GetNumberOfValues() != 1)
    {
      throw svtkm::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.GetPortalConstControl().Get(0);
  }
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT void HistogramMPI::PostExecute(const svtkm::cont::PartitionedDataSet&,
                                                svtkm::cont::PartitionedDataSet& result,
                                                const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  // iterate and compute HistogramMPI for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (svtkm::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  svtkm::cont::DataSet output;
  svtkm::cont::Field rfield(this->GetOutputFieldName(),
                           svtkm::cont::Field::Association::WHOLE_MESH,
                           helper.ReduceAll(this->NumberOfBins));
  output.AddField(rfield);

  result = svtkm::cont::PartitionedDataSet(output);
}
} // namespace example
