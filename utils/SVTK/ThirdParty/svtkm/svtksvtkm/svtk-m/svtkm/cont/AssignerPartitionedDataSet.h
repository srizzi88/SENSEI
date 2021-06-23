//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_AssignerPartitionedDataSet_h
#define svtk_m_cont_AssignerPartitionedDataSet_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>
#include <svtkm/thirdparty/diy/Configure.h>

#include <vector>

#include <svtkm/thirdparty/diy/diy.h>

#ifdef SVTKM_MSVC
#pragma warning(push)
// disable C4275: non-dll interface base class warnings
#pragma warning(disable : 4275)
#endif

namespace svtkm
{
namespace cont
{

class PartitionedDataSet;

/// \brief Assigner for PartitionedDataSet partitions.
///
/// `AssignerPartitionedDataSet` is a `svtkmdiy::StaticAssigner` implementation
/// that uses `PartitionedDataSet`'s partition distribution to build
/// global-id/rank associations needed for several `diy` operations.
/// It uses a contiguous assignment strategy to map partitions to global ids,
/// i.e. partitions on rank 0 come first, then rank 1, etc. Any rank may have 0
/// partitions.
///
/// AssignerPartitionedDataSet uses collectives in the constructor hence it is
/// essential it gets created on all ranks irrespective of whether the rank has
/// any partitions.
///
class SVTKM_CONT_EXPORT AssignerPartitionedDataSet : public svtkmdiy::StaticAssigner
{
public:
  /// Initialize the assigner using a partitioned dataset.
  /// This may initialize collective operations to populate the assigner with
  /// information about partitions on all ranks.
  SVTKM_CONT
  AssignerPartitionedDataSet(const svtkm::cont::PartitionedDataSet& pds);

  SVTKM_CONT
  AssignerPartitionedDataSet(svtkm::Id num_partitions);

  SVTKM_CONT
  virtual ~AssignerPartitionedDataSet();

  ///@{
  /// svtkmdiy::Assigner API implementation.
  SVTKM_CONT
  void local_gids(int rank, std::vector<int>& gids) const override;

  SVTKM_CONT
  int rank(int gid) const override;
  //@}
private:
  std::vector<svtkm::Id> IScanPartitionCounts;
};
}
}

#ifdef SVTKM_MSVC
#pragma warning(pop)
#endif

#endif
