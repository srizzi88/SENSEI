//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/StaticAssert.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/EnvironmentTracker.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/PartitionedDataSet.h>

namespace svtkm
{
namespace cont
{

SVTKM_CONT
PartitionedDataSet::PartitionedDataSet(const svtkm::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

SVTKM_CONT
PartitionedDataSet::PartitionedDataSet(const svtkm::cont::PartitionedDataSet& src)
{
  this->Partitions = src.GetPartitions();
}

SVTKM_CONT
PartitionedDataSet::PartitionedDataSet(const std::vector<svtkm::cont::DataSet>& partitions)
{
  this->Partitions = partitions;
}

SVTKM_CONT
PartitionedDataSet::PartitionedDataSet(svtkm::Id size)
{
  this->Partitions.reserve(static_cast<std::size_t>(size));
}

SVTKM_CONT
PartitionedDataSet::PartitionedDataSet()
{
}

SVTKM_CONT
PartitionedDataSet::~PartitionedDataSet()
{
}

SVTKM_CONT
PartitionedDataSet& PartitionedDataSet::operator=(const svtkm::cont::PartitionedDataSet& src)
{
  this->Partitions = src.GetPartitions();
  return *this;
}

SVTKM_CONT
svtkm::cont::Field PartitionedDataSet::GetField(const std::string& field_name, int partition_index)
{
  assert(partition_index >= 0);
  assert(static_cast<std::size_t>(partition_index) < this->Partitions.size());
  return this->Partitions[static_cast<std::size_t>(partition_index)].GetField(field_name);
}

SVTKM_CONT
svtkm::Id PartitionedDataSet::GetNumberOfPartitions() const
{
  return static_cast<svtkm::Id>(this->Partitions.size());
}

SVTKM_CONT
const svtkm::cont::DataSet& PartitionedDataSet::GetPartition(svtkm::Id blockId) const
{
  return this->Partitions[static_cast<std::size_t>(blockId)];
}

SVTKM_CONT
const std::vector<svtkm::cont::DataSet>& PartitionedDataSet::GetPartitions() const
{
  return this->Partitions;
}

SVTKM_CONT
void PartitionedDataSet::AppendPartition(const svtkm::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

SVTKM_CONT
void PartitionedDataSet::AppendPartitions(const std::vector<svtkm::cont::DataSet>& partitions)
{
  this->Partitions.insert(this->Partitions.end(), partitions.begin(), partitions.end());
}

SVTKM_CONT
void PartitionedDataSet::InsertPartition(svtkm::Id index, const svtkm::cont::DataSet& ds)
{
  if (index <= static_cast<svtkm::Id>(this->Partitions.size()))
  {
    this->Partitions.insert(this->Partitions.begin() + index, ds);
  }
  else
  {
    std::string msg = "invalid insert position\n ";
    throw ErrorBadValue(msg);
  }
}

SVTKM_CONT
void PartitionedDataSet::ReplacePartition(svtkm::Id index, const svtkm::cont::DataSet& ds)
{
  if (index < static_cast<svtkm::Id>(this->Partitions.size()))
    this->Partitions.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorBadValue(msg);
  }
}

SVTKM_CONT
void PartitionedDataSet::PrintSummary(std::ostream& stream) const
{
  stream << "PartitionedDataSet [" << this->Partitions.size() << " partitions]:\n";

  for (size_t part = 0; part < this->Partitions.size(); ++part)
  {
    stream << "Partition " << part << ":\n";
    this->Partitions[part].PrintSummary(stream);
  }
}
}
} // namespace svtkm::cont
