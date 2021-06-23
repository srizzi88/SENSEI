//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/CellShape.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetFieldAdd.h>

#include <svtkm/cont/PartitionedDataSet.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/exec/ConnectivityStructured.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/CellAverage.h>


template <typename T>
svtkm::cont::PartitionedDataSet PartitionedDataSetBuilder(std::size_t partitionNum,
                                                         std::string fieldName)
{
  svtkm::cont::DataSetBuilderUniform dataSetBuilder;
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetFieldAdd dsf;

  svtkm::Vec<T, 2> origin(0);
  svtkm::Vec<T, 2> spacing(1);
  svtkm::cont::PartitionedDataSet partitions;
  for (svtkm::Id partId = 0; partId < static_cast<svtkm::Id>(partitionNum); partId++)
  {
    svtkm::Id2 dimensions((partId + 2) * (partId + 2), (partId + 2) * (partId + 2));

    if (fieldName == "cellvar")
    {
      svtkm::Id numCells = (dimensions[0] - 1) * (dimensions[1] - 1);

      std::vector<T> varC2D(static_cast<std::size_t>(numCells));
      for (svtkm::Id i = 0; i < numCells; i++)
      {
        varC2D[static_cast<std::size_t>(i)] = static_cast<T>(partId * i);
      }
      dataSet = dataSetBuilder.Create(svtkm::Id2(dimensions[0], dimensions[1]),
                                      svtkm::Vec<T, 2>(origin[0], origin[1]),
                                      svtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddCellField(dataSet, "cellvar", varC2D);
    }

    if (fieldName == "pointvar")
    {
      svtkm::Id numPoints = dimensions[0] * dimensions[1];
      std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
      for (svtkm::Id i = 0; i < numPoints; i++)
      {
        varP2D[static_cast<std::size_t>(i)] = static_cast<T>(partId);
      }
      dataSet = dataSetBuilder.Create(svtkm::Id2(dimensions[0], dimensions[1]),
                                      svtkm::Vec<T, 2>(origin[0], origin[1]),
                                      svtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddPointField(dataSet, "pointvar", varP2D);
    }

    partitions.AppendPartition(dataSet);
  }
  return partitions;
}
template <typename D>
void Result_Verify(const svtkm::cont::PartitionedDataSet& result,
                   D& filter,
                   const svtkm::cont::PartitionedDataSet& partitions,
                   std::string fieldName)
{
  SVTKM_TEST_ASSERT(result.GetNumberOfPartitions() == partitions.GetNumberOfPartitions(),
                   "result partition number incorrect");
  const std::string outputFieldName = filter.GetOutputFieldName();
  for (svtkm::Id j = 0; j < result.GetNumberOfPartitions(); j++)
  {
    filter.SetActiveField(fieldName);
    svtkm::cont::DataSet partitionResult = filter.Execute(partitions.GetPartition(j));

    SVTKM_TEST_ASSERT(result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues() ==
                       partitionResult.GetField(outputFieldName).GetNumberOfValues(),
                     "result vectors' size incorrect");

    svtkm::cont::ArrayHandle<svtkm::Id> partitionArray;
    result.GetPartition(j).GetField(outputFieldName).GetData().CopyTo(partitionArray);
    svtkm::cont::ArrayHandle<svtkm::Id> sDataSetArray;
    partitionResult.GetField(outputFieldName).GetData().CopyTo(sDataSetArray);

    const svtkm::Id numValues = result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues();
    for (svtkm::Id i = 0; i < numValues; i++)
    {
      SVTKM_TEST_ASSERT(partitionArray.GetPortalConstControl().Get(i) ==
                         sDataSetArray.GetPortalConstControl().Get(i),
                       "result values incorrect");
    }
  }
  return;
}

void TestPartitionedDataSetFilters()
{
  std::size_t partitionNum = 7;
  svtkm::cont::PartitionedDataSet result;
  svtkm::cont::PartitionedDataSet partitions;

  partitions = PartitionedDataSetBuilder<svtkm::Id>(partitionNum, "pointvar");
  svtkm::filter::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("average");
  cellAverage.SetActiveField("pointvar");
  result = cellAverage.Execute(partitions);
  Result_Verify(result, cellAverage, partitions, std::string("pointvar"));
}

int UnitTestPartitionedDataSetFilters(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestPartitionedDataSetFilters, argc, argv);
}
