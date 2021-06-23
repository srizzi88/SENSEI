//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/FieldRangeGlobalCompute.h>

#include <svtkm/cont/EnvironmentTracker.h>

#include <svtkm/thirdparty/diy/diy.h>

#include <algorithm>
#include <functional>

namespace svtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(const svtkm::cont::DataSet& dataset,
                                                             const std::string& name,
                                                             svtkm::cont::Field::Association assoc)
{
  return detail::FieldRangeGlobalComputeImpl(dataset, name, assoc, SVTKM_DEFAULT_TYPE_LIST());
}

//-----------------------------------------------------------------------------
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> FieldRangeGlobalCompute(
  const svtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  svtkm::cont::Field::Association assoc)
{
  return detail::FieldRangeGlobalComputeImpl(pds, name, assoc, SVTKM_DEFAULT_TYPE_LIST());
}

//-----------------------------------------------------------------------------
namespace detail
{
SVTKM_CONT
svtkm::cont::ArrayHandle<svtkm::Range> MergeRangesGlobal(
  const svtkm::cont::ArrayHandle<svtkm::Range>& ranges)
{
  auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() == 1)
  {
    return ranges;
  }

  std::vector<svtkm::Range> v_ranges(static_cast<size_t>(ranges.GetNumberOfValues()));
  std::copy(svtkm::cont::ArrayPortalToIteratorBegin(ranges.GetPortalConstControl()),
            svtkm::cont::ArrayPortalToIteratorEnd(ranges.GetPortalConstControl()),
            v_ranges.begin());

  using VectorOfRangesT = std::vector<svtkm::Range>;

  svtkmdiy::Master master(comm,
                         1,
                         -1,
                         []() -> void* { return new VectorOfRangesT(); },
                         [](void* ptr) { delete static_cast<VectorOfRangesT*>(ptr); });

  svtkmdiy::ContiguousAssigner assigner(/*num ranks*/ comm.size(),
                                       /*global-num-blocks*/ comm.size());
  svtkmdiy::RegularDecomposer<svtkmdiy::DiscreteBounds> decomposer(
    /*dim*/ 1, svtkmdiy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1); // each rank will have exactly 1 block.
  *master.block<VectorOfRangesT>(0) = v_ranges;

  svtkmdiy::RegularAllReducePartners all_reduce_partners(decomposer, /*k*/ 2);

  auto callback = [](
    VectorOfRangesT* data, const svtkmdiy::ReduceProxy& srp, const svtkmdiy::RegularMergePartners&) {
    const auto selfid = srp.gid();
    // 1. dequeue.
    std::vector<int> incoming;
    srp.incoming(incoming);
    for (const int gid : incoming)
    {
      if (gid != selfid)
      {
        VectorOfRangesT message;
        srp.dequeue(gid, message);

        // if the number of components we've seen so far is less than those
        // in the received message, resize so we can accommodate all components
        // in the message. If the message has fewer components, it has no
        // effect.
        data->resize(std::max(data->size(), message.size()));

        std::transform(
          message.begin(), message.end(), data->begin(), data->begin(), std::plus<svtkm::Range>());
      }
    }
    // 2. enqueue
    for (int cc = 0; cc < srp.out_link().size(); ++cc)
    {
      auto target = srp.out_link().target(cc);
      if (target.gid != selfid)
      {
        srp.enqueue(target, *data);
      }
    }
  };

  svtkmdiy::reduce(master, assigner, all_reduce_partners, callback);
  assert(master.size() == 1); // each rank will have exactly 1 block.

  return svtkm::cont::make_ArrayHandle(*master.block<VectorOfRangesT>(0), svtkm::CopyFlag::On);
}
} // namespace detail
}
} // namespace svtkm::cont
