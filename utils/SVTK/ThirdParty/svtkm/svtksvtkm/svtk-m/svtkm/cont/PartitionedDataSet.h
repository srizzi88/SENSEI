//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_PartitionedDataSet_h
#define svtk_m_cont_PartitionedDataSet_h
#include <limits>
#include <svtkm/StaticAssert.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT PartitionedDataSet
{
  using StorageVec = std::vector<svtkm::cont::DataSet>;

public:
  using iterator = typename StorageVec::iterator;
  using const_iterator = typename StorageVec::const_iterator;
  using value_type = typename StorageVec::value_type;
  using reference = typename StorageVec::reference;
  using const_reference = typename StorageVec::const_reference;

  /// create a new PartitionedDataSet containng a single DataSet @a ds
  SVTKM_CONT
  PartitionedDataSet(const svtkm::cont::DataSet& ds);
  /// create a new PartitionedDataSet with the existing one @a src
  SVTKM_CONT
  PartitionedDataSet(const svtkm::cont::PartitionedDataSet& src);
  /// create a new PartitionedDataSet with a DataSet vector @a partitions.
  SVTKM_CONT
  explicit PartitionedDataSet(const std::vector<svtkm::cont::DataSet>& partitions);
  /// create a new PartitionedDataSet with the capacity set to be @a size
  SVTKM_CONT
  explicit PartitionedDataSet(svtkm::Id size);

  SVTKM_CONT
  PartitionedDataSet();

  SVTKM_CONT
  PartitionedDataSet& operator=(const svtkm::cont::PartitionedDataSet& src);

  SVTKM_CONT
  ~PartitionedDataSet();
  /// get the field @a field_name from partition @a partition_index
  SVTKM_CONT
  svtkm::cont::Field GetField(const std::string& field_name, int partition_index);

  SVTKM_CONT
  svtkm::Id GetNumberOfPartitions() const;

  SVTKM_CONT
  const svtkm::cont::DataSet& GetPartition(svtkm::Id partId) const;

  SVTKM_CONT
  const std::vector<svtkm::cont::DataSet>& GetPartitions() const;

  /// add DataSet @a ds to the end of the contained DataSet vector
  SVTKM_CONT
  void AppendPartition(const svtkm::cont::DataSet& ds);

  /// add DataSet @a ds to position @a index of the contained DataSet vector
  SVTKM_CONT
  void InsertPartition(svtkm::Id index, const svtkm::cont::DataSet& ds);

  /// replace the @a index positioned element of the contained DataSet vector
  /// with @a ds
  SVTKM_CONT
  void ReplacePartition(svtkm::Id index, const svtkm::cont::DataSet& ds);

  /// append the DataSet vector "partitions"  to the end of the contained one
  SVTKM_CONT
  void AppendPartitions(const std::vector<svtkm::cont::DataSet>& partitions);

  SVTKM_CONT
  void PrintSummary(std::ostream& stream) const;

  //@{
  /// API to support range-based for loops on partitions.
  SVTKM_CONT
  iterator begin() noexcept { return this->Partitions.begin(); }
  SVTKM_CONT
  iterator end() noexcept { return this->Partitions.end(); }
  SVTKM_CONT
  const_iterator begin() const noexcept { return this->Partitions.begin(); }
  SVTKM_CONT
  const_iterator end() const noexcept { return this->Partitions.end(); }
  SVTKM_CONT
  const_iterator cbegin() const noexcept { return this->Partitions.cbegin(); }
  SVTKM_CONT
  const_iterator cend() const noexcept { return this->Partitions.cend(); }
  //@}
private:
  std::vector<svtkm::cont::DataSet> Partitions;
};
}
} // namespace svtkm::cont

#endif
