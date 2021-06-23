//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Histogram_hxx
#define svtk_m_filter_Histogram_hxx

#include <svtkm/worklet/FieldHistogram.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/AssignerPartitionedDataSet.h>
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/FieldRangeGlobalCompute.h>
#include <svtkm/cont/Serialization.h>

#include <svtkm/thirdparty/diy/diy.h>

namespace svtkm
{
namespace filter
{
namespace detail
{
class DistributedHistogram
{
  class Reducer
  {
  public:
    void operator()(svtkm::cont::ArrayHandle<svtkm::Id>* result,
                    const svtkmdiy::ReduceProxy& srp,
                    const svtkmdiy::RegularMergePartners&) const
    {
      const auto selfid = srp.gid();
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      for (const int gid : incoming)
      {
        if (gid != selfid)
        {
          svtkm::cont::ArrayHandle<svtkm::Id> in;
          srp.dequeue(gid, in);
          if (result->GetNumberOfValues() == 0)
          {
            *result = in;
          }
          else
          {
            svtkm::cont::Algorithm::Transform(*result, in, *result, svtkm::Add());
          }
        }
      }

      // 2. enqueue
      for (int cc = 0; cc < srp.out_link().size(); ++cc)
      {
        auto target = srp.out_link().target(cc);
        if (target.gid != selfid)
        {
          srp.enqueue(target, *result);
        }
      }
    }
  };

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

  svtkm::cont::ArrayHandle<svtkm::Id> ReduceAll() const
  {
    using ArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;

    const svtkm::Id numLocalBlocks = static_cast<svtkm::Id>(this->LocalBlocks.size());
    auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() == 1 && numLocalBlocks <= 1)
    {
      // no reduction necessary.
      return numLocalBlocks == 0 ? ArrayType() : this->LocalBlocks[0];
    }

    svtkmdiy::Master master(
      comm,
      /*threads*/ 1,
      /*limit*/ -1,
      []() -> void* { return new svtkm::cont::ArrayHandle<svtkm::Id>(); },
      [](void* ptr) { delete static_cast<svtkm::cont::ArrayHandle<svtkm::Id>*>(ptr); });

    svtkm::cont::AssignerPartitionedDataSet assigner(numLocalBlocks);
    svtkmdiy::RegularDecomposer<svtkmdiy::DiscreteBounds> decomposer(
      /*dims*/ 1, svtkmdiy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
    decomposer.decompose(comm.rank(), assigner, master);

    assert(static_cast<svtkm::Id>(master.size()) == numLocalBlocks);
    for (svtkm::Id cc = 0; cc < numLocalBlocks; ++cc)
    {
      *master.block<ArrayType>(static_cast<int>(cc)) = this->LocalBlocks[static_cast<size_t>(cc)];
    }

    svtkmdiy::RegularMergePartners partners(decomposer, /*k=*/2);
    // reduce to block-0.
    svtkmdiy::reduce(master, assigner, partners, Reducer());

    ArrayType result;
    if (master.local(0))
    {
      result = *master.block<ArrayType>(master.lid(0));
    }

    this->Broadcast(result);
    return result;
  }

private:
  void Broadcast(svtkm::cont::ArrayHandle<svtkm::Id>& data) const
  {
    // broadcast to all ranks (and not blocks).
    auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
    if (comm.size() > 1)
    {
      using ArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;
      svtkmdiy::Master master(
        comm,
        /*threads*/ 1,
        /*limit*/ -1,
        []() -> void* { return new svtkm::cont::ArrayHandle<svtkm::Id>(); },
        [](void* ptr) { delete static_cast<svtkm::cont::ArrayHandle<svtkm::Id>*>(ptr); });

      svtkmdiy::ContiguousAssigner assigner(comm.size(), comm.size());
      svtkmdiy::RegularDecomposer<svtkmdiy::DiscreteBounds> decomposer(
        1, svtkmdiy::interval(0, comm.size() - 1), comm.size());
      decomposer.decompose(comm.rank(), assigner, master);
      assert(master.size() == 1); // number of local blocks should be 1 per rank.
      *master.block<ArrayType>(0) = data;
      svtkmdiy::RegularBroadcastPartners partners(decomposer, /*k=*/2);
      svtkmdiy::reduce(master, assigner, partners, Reducer());
      data = *master.block<ArrayType>(0);
    }
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline SVTKM_CONT Histogram::Histogram()
  : NumberOfBins(10)
  , BinDelta(0)
  , ComputedRange()
  , Range()
{
  this->SetOutputFieldName("histogram");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Histogram::DoExecute(
  const svtkm::cont::DataSet&,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata&,
  svtkm::filter::PolicyBase<DerivedPolicy>)
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
inline SVTKM_CONT void Histogram::PreExecute(const svtkm::cont::PartitionedDataSet& input,
                                            const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using TypeList = typename DerivedPolicy::FieldTypeList;
  if (this->Range.IsNonEmpty())
  {
    this->ComputedRange = this->Range;
  }
  else
  {
    auto handle = svtkm::cont::FieldRangeGlobalCompute(
      input, this->GetActiveFieldName(), this->GetActiveFieldAssociation(), TypeList());
    if (handle.GetNumberOfValues() != 1)
    {
      throw svtkm::cont::ErrorFilterExecution("expecting scalar field.");
    }
    this->ComputedRange = handle.GetPortalConstControl().Get(0);
  }
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline SVTKM_CONT void Histogram::PostExecute(const svtkm::cont::PartitionedDataSet&,
                                             svtkm::cont::PartitionedDataSet& result,
                                             const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  // iterate and compute histogram for each local block.
  detail::DistributedHistogram helper(result.GetNumberOfPartitions());
  for (svtkm::Id cc = 0; cc < result.GetNumberOfPartitions(); ++cc)
  {
    auto& ablock = result.GetPartition(cc);
    helper.SetLocalHistogram(cc, ablock.GetField(this->GetOutputFieldName()));
  }

  svtkm::cont::DataSet output;
  svtkm::cont::Field rfield(
    this->GetOutputFieldName(), svtkm::cont::Field::Association::WHOLE_MESH, helper.ReduceAll());
  output.AddField(rfield);

  result = svtkm::cont::PartitionedDataSet(output);
}
}
} // namespace svtkm::filter

#endif
