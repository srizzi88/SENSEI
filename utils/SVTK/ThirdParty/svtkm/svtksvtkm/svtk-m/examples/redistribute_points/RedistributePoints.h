//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/ImplicitFunction.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/AssignerPartitionedDataSet.h>
#include <svtkm/cont/BoundsGlobalCompute.h>
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/Serialization.h>
#include <svtkm/filter/ExtractPoints.h>
#include <svtkm/filter/Filter.h>

#include <svtkm/thirdparty/diy/diy.h>

namespace example
{

namespace internal
{

static svtkmdiy::ContinuousBounds convert(const svtkm::Bounds& bds)
{
  svtkmdiy::ContinuousBounds result;
  result.min[0] = static_cast<float>(bds.X.Min);
  result.min[1] = static_cast<float>(bds.Y.Min);
  result.min[2] = static_cast<float>(bds.Z.Min);
  result.max[0] = static_cast<float>(bds.X.Max);
  result.max[1] = static_cast<float>(bds.Y.Max);
  result.max[2] = static_cast<float>(bds.Z.Max);
  return result;
}


template <typename DerivedPolicy>
class Redistributor
{
  const svtkmdiy::RegularDecomposer<svtkmdiy::ContinuousBounds>& Decomposer;
  const svtkm::filter::PolicyBase<DerivedPolicy>& Policy;

  svtkm::cont::DataSet Extract(const svtkm::cont::DataSet& input,
                              const svtkmdiy::ContinuousBounds& bds) const
  {
    // extract points
    svtkm::Box box(bds.min[0], bds.max[0], bds.min[1], bds.max[1], bds.min[2], bds.max[2]);

    svtkm::filter::ExtractPoints extractor;
    extractor.SetCompactPoints(true);
    extractor.SetImplicitFunction(svtkm::cont::make_ImplicitFunctionHandle(box));
    return extractor.Execute(input, this->Policy);
  }

  class ConcatenateFields
  {
  public:
    explicit ConcatenateFields(svtkm::Id totalSize)
      : TotalSize(totalSize)
      , CurrentIdx(0)
    {
    }

    void Append(const svtkm::cont::Field& field)
    {
      SVTKM_ASSERT(this->CurrentIdx + field.GetNumberOfValues() <= this->TotalSize);

      if (this->Field.GetNumberOfValues() == 0)
      {
        this->Field = field;
        field.GetData().CastAndCall(Allocator{}, this->Field, this->TotalSize);
      }
      else
      {
        SVTKM_ASSERT(this->Field.GetName() == field.GetName() &&
                    this->Field.GetAssociation() == field.GetAssociation());
      }

      field.GetData().CastAndCall(Appender{}, this->Field, this->CurrentIdx);
      this->CurrentIdx += field.GetNumberOfValues();
    }

    const svtkm::cont::Field& GetResult() const { return this->Field; }

  private:
    struct Allocator
    {
      template <typename T, typename S>
      void operator()(const svtkm::cont::ArrayHandle<T, S>&,
                      svtkm::cont::Field& field,
                      svtkm::Id totalSize) const
      {
        svtkm::cont::ArrayHandle<T> init;
        init.Allocate(totalSize);
        field.SetData(init);
      }
    };

    struct Appender
    {
      template <typename T, typename S>
      void operator()(const svtkm::cont::ArrayHandle<T, S>& data,
                      svtkm::cont::Field& field,
                      svtkm::Id currentIdx) const
      {
        svtkm::cont::ArrayHandle<T> farray =
          field.GetData().template Cast<svtkm::cont::ArrayHandle<T>>();
        svtkm::cont::Algorithm::CopySubRange(data, 0, data.GetNumberOfValues(), farray, currentIdx);
      }
    };

    svtkm::Id TotalSize;
    svtkm::Id CurrentIdx;
    svtkm::cont::Field Field;
  };

public:
  Redistributor(const svtkmdiy::RegularDecomposer<svtkmdiy::ContinuousBounds>& decomposer,
                const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
    : Decomposer(decomposer)
    , Policy(policy)
  {
  }

  void operator()(svtkm::cont::DataSet* block, const svtkmdiy::ReduceProxy& rp) const
  {
    if (rp.in_link().size() == 0)
    {
      if (block->GetNumberOfCoordinateSystems() > 0)
      {
        for (int cc = 0; cc < rp.out_link().size(); ++cc)
        {
          auto target = rp.out_link().target(cc);
          // let's get the bounding box for the target block.
          svtkmdiy::ContinuousBounds bds;
          this->Decomposer.fill_bounds(bds, target.gid);

          auto extractedDS = this->Extract(*block, bds);
          rp.enqueue(target, svtkm::filter::MakeSerializableDataSet(extractedDS, DerivedPolicy{}));
        }
        // clear our dataset.
        *block = svtkm::cont::DataSet();
      }
    }
    else
    {
      svtkm::Id numValues = 0;
      std::vector<svtkm::cont::DataSet> receives;
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        auto target = rp.in_link().target(cc);
        if (rp.incoming(target.gid).size() > 0)
        {
          auto sds = svtkm::filter::MakeSerializableDataSet(DerivedPolicy{});
          rp.dequeue(target.gid, sds);
          receives.push_back(sds.DataSet);
          numValues += receives.back().GetCoordinateSystem(0).GetNumberOfPoints();
        }
      }

      *block = svtkm::cont::DataSet();
      if (receives.size() == 1)
      {
        *block = receives[0];
      }
      else if (receives.size() > 1)
      {
        ConcatenateFields concatCoords(numValues);
        for (const auto& ds : receives)
        {
          concatCoords.Append(ds.GetCoordinateSystem(0));
        }
        block->AddCoordinateSystem(svtkm::cont::CoordinateSystem(
          concatCoords.GetResult().GetName(), concatCoords.GetResult().GetData()));

        for (svtkm::IdComponent i = 0; i < receives[0].GetNumberOfFields(); ++i)
        {
          ConcatenateFields concatField(numValues);
          for (const auto& ds : receives)
          {
            concatField.Append(ds.GetField(i));
          }
          block->AddField(concatField.GetResult());
        }
      }
    }
  }
};

} // namespace example::internal


class RedistributePoints : public svtkm::filter::Filter<RedistributePoints>
{
public:
  SVTKM_CONT
  RedistributePoints() {}

  SVTKM_CONT
  ~RedistributePoints() {}

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::PartitionedDataSet PrepareForExecution(
    const svtkm::cont::PartitionedDataSet& input,
    const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
};

template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::PartitionedDataSet RedistributePoints::PrepareForExecution(
  const svtkm::cont::PartitionedDataSet& input,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();

  // let's first get the global bounds of the domain
  svtkm::Bounds gbounds = svtkm::cont::BoundsGlobalCompute(input);

  svtkm::cont::AssignerPartitionedDataSet assigner(input.GetNumberOfPartitions());
  svtkmdiy::RegularDecomposer<svtkmdiy::ContinuousBounds> decomposer(
    /*dim*/ 3, internal::convert(gbounds), assigner.nblocks());

  svtkmdiy::Master master(comm,
                         /*threads*/ 1,
                         /*limit*/ -1,
                         []() -> void* { return new svtkm::cont::DataSet(); },
                         [](void* ptr) { delete static_cast<svtkm::cont::DataSet*>(ptr); });
  decomposer.decompose(comm.rank(), assigner, master);

  assert(static_cast<svtkm::Id>(master.size()) == input.GetNumberOfPartitions());
  // let's populate local blocks
  master.foreach ([&input](svtkm::cont::DataSet* ds, const svtkmdiy::Master::ProxyWithLink& proxy) {
    auto lid = proxy.master()->lid(proxy.gid());
    *ds = input.GetPartition(lid);
  });

  internal::Redistributor<DerivedPolicy> redistributor(decomposer, policy);
  svtkmdiy::all_to_all(master, assigner, redistributor, /*k=*/2);

  svtkm::cont::PartitionedDataSet result;
  master.foreach ([&result](svtkm::cont::DataSet* ds, const svtkmdiy::Master::ProxyWithLink&) {
    result.AppendPartition(*ds);
  });

  return result;
}

} // namespace example
