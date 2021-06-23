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

#include <svtkm/Bounds.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/BoundsCompute.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/FieldRangeCompute.h>
#include <svtkm/cont/PartitionedDataSet.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/exec/ConnectivityStructured.h>
#include <svtkm/thirdparty/diy/Configure.h>

#include <svtkm/thirdparty/diy/diy.h>

void DataSet_Compare(svtkm::cont::DataSet& LeftDateSet, svtkm::cont::DataSet& RightDateSet);
static void PartitionedDataSetTest()
{
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::PartitionedDataSet pds;

  svtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  svtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  pds.AppendPartition(TDset1);
  pds.AppendPartition(TDset2);

  SVTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");

  svtkm::cont::DataSet TestDSet = pds.GetPartition(0);
  SVTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  SVTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  TestDSet = pds.GetPartition(1);
  SVTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  SVTKM_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  svtkm::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  svtkm::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  svtkm::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  SVTKM_TEST_ASSERT(svtkm::cont::BoundsCompute(pds) == GlobalBound, "Global bounds info incorrect");
  SVTKM_TEST_ASSERT(svtkm::cont::BoundsCompute(pds.GetPartition(0)) == Set1Bounds,
                   "Local bounds info incorrect");
  SVTKM_TEST_ASSERT(svtkm::cont::BoundsCompute(pds.GetPartition(1)) == Set2Bounds,
                   "Local bounds info incorrect");

  svtkm::Range Set1Field1Range;
  svtkm::Range Set1Field2Range;
  svtkm::Range Set2Field1Range;
  svtkm::Range Set2Field2Range;
  svtkm::Range Field1GlobeRange;
  svtkm::Range Field2GlobeRange;

  TDset1.GetField("pointvar").GetRange(&Set1Field1Range);
  TDset1.GetField("cellvar").GetRange(&Set1Field2Range);
  TDset2.GetField("pointvar").GetRange(&Set2Field1Range);
  TDset2.GetField("cellvar").GetRange(&Set2Field2Range);

  Field1GlobeRange.Include(Set1Field1Range);
  Field1GlobeRange.Include(Set2Field1Range);
  Field2GlobeRange.Include(Set1Field2Range);
  Field2GlobeRange.Include(Set2Field2Range);

  using svtkm::cont::FieldRangeCompute;
  SVTKM_TEST_ASSERT(FieldRangeCompute(pds, "pointvar").GetPortalConstControl().Get(0) ==
                     Field1GlobeRange,
                   "Local field value range info incorrect");
  SVTKM_TEST_ASSERT(FieldRangeCompute(pds, "cellvar").GetPortalConstControl().Get(0) ==
                     Field2GlobeRange,
                   "Local field value range info incorrect");

  svtkm::Range SourceRange; //test the validity of member function GetField(FieldName, BlockId)
  pds.GetField("cellvar", 0).GetRange(&SourceRange);
  svtkm::Range TestRange;
  pds.GetPartition(0).GetField("cellvar").GetRange(&TestRange);
  SVTKM_TEST_ASSERT(TestRange == SourceRange, "Local field value info incorrect");

  svtkm::cont::PartitionedDataSet testblocks1;
  std::vector<svtkm::cont::DataSet> partitions = pds.GetPartitions();
  testblocks1.AppendPartitions(partitions);
  SVTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == testblocks1.GetNumberOfPartitions(),
                   "inconsistent number of partitions");

  svtkm::cont::PartitionedDataSet testblocks2(2);
  testblocks2.InsertPartition(0, TDset1);
  testblocks2.InsertPartition(1, TDset2);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset1, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset2, TestDSet);

  testblocks2.ReplacePartition(0, TDset2);
  testblocks2.ReplacePartition(1, TDset1);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset2, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset1, TestDSet);
}

void DataSet_Compare(svtkm::cont::DataSet& leftDataSet, svtkm::cont::DataSet& rightDataSet)
{
  for (svtkm::Id j = 0; j < leftDataSet.GetNumberOfFields(); j++)
  {
    svtkm::cont::ArrayHandle<svtkm::Float32> lDataArray;
    leftDataSet.GetField(j).GetData().CopyTo(lDataArray);
    svtkm::cont::ArrayHandle<svtkm::Float32> rDataArray;
    rightDataSet.GetField(j).GetData().CopyTo(rDataArray);
    SVTKM_TEST_ASSERT(lDataArray == rDataArray, "field value info incorrect");
  }
  return;
}

int UnitTestPartitionedDataSet(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(PartitionedDataSetTest, argc, argv);
}
