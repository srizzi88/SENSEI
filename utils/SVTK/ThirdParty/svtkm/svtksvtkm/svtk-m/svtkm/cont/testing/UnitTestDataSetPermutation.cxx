//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/worklet/CellAverage.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace
{

template <typename T, typename Storage>
bool TestArrayHandle(const svtkm::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     svtkm::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  for (svtkm::Id i = 0; i < size; ++i)
  {
    if (ah.GetPortalConstControl().Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

inline svtkm::cont::DataSet make_SingleTypeDataSet()
{
  using CoordType = svtkm::Vec3f_32;
  std::vector<CoordType> coordinates;
  coordinates.push_back(CoordType(0, 0, 0));
  coordinates.push_back(CoordType(1, 0, 0));
  coordinates.push_back(CoordType(1, 1, 0));
  coordinates.push_back(CoordType(2, 1, 0));
  coordinates.push_back(CoordType(2, 2, 0));

  std::vector<svtkm::Id> conn;
  // First Cell
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);
  // Third Cell
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  svtkm::cont::DataSet ds;
  svtkm::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coordinates, svtkm::CellShapeTagTriangle(), 3, conn);

  //Set point scalar
  const int nVerts = 5;
  svtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  svtkm::cont::DataSetFieldAdd::AddPointField(ds, "pointvar", vars, nVerts);

  return ds;
}

void TestDataSet_Explicit()
{

  svtkm::cont::DataSet dataSet = make_SingleTypeDataSet();

  std::vector<svtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  svtkm::cont::ArrayHandle<svtkm::Id> validCellIds = svtkm::cont::make_ArrayHandle(validIds);

  //get the cellset single type from the dataset
  svtkm::cont::CellSetSingleType<> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a singlset
  using SubsetType = svtkm::cont::CellSetPermutation<svtkm::cont::CellSetSingleType<>>;
  SubsetType subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  using ExecObjectType = SubsetType::ExecutionTypes<svtkm::cont::DeviceAdapterTagSerial,
                                                    svtkm::TopologyElementTagCell,
                                                    svtkm::TopologyElementTagPoint>::ExecObjectType;

  ExecObjectType execConnectivity;
  execConnectivity = subset.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                                            svtkm::TopologyElementTagCell(),
                                            svtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  svtkm::cont::ArrayHandle<svtkm::Float32> result;
  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  //iterate same cell 4 times
  svtkm::Float32 expected[4] = { 30.1667f, 30.1667f, 30.1667f, 30.1667f };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on explicit subset data");
  }
}

void TestDataSet_Structured2D()
{

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  std::vector<svtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  svtkm::cont::ArrayHandle<svtkm::Id> validCellIds = svtkm::cont::make_ArrayHandle(validIds);

  svtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<2>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //verify that we can call PrepareForInput on CellSetSingleType
  using DeviceAdapterTag = svtkm::cont::DeviceAdapterTagSerial;

  //verify that PrepareForInput exists
  subset.PrepareForInput(
    DeviceAdapterTag(), svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  svtkm::cont::ArrayHandle<svtkm::Float32> result;
  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  svtkm::Float32 expected[4] = { 40.1f, 40.1f, 40.1f, 40.1f };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Structured3D()
{

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  std::vector<svtkm::Id> validIds;
  validIds.push_back(1); //iterate the 2nd cell 4 times
  validIds.push_back(1);
  validIds.push_back(1);
  validIds.push_back(1);
  svtkm::cont::ArrayHandle<svtkm::Id> validCellIds = svtkm::cont::make_ArrayHandle(validIds);

  svtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  //verify that we can create a subset of a 2d UniformDataSet
  svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<3>> subset;
  subset.Fill(validCellIds, cellSet);

  subset.PrintSummary(std::cout);

  //verify that PrepareForInput exists
  subset.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                         svtkm::TopologyElementTagCell(),
                         svtkm::TopologyElementTagPoint());

  //run a basic for-each topology algorithm on this
  svtkm::cont::ArrayHandle<svtkm::Float32> result;
  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(subset, dataSet.GetField("pointvar"), result);

  svtkm::Float32 expected[4] = { 70.2125f, 70.2125f, 70.2125f, 70.2125f };
  for (int i = 0; i < 4; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for CellAverage worklet on 2d structured subset data");
  }
}

void TestDataSet_Permutation()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Permutation--" << std::endl << std::endl;

  TestDataSet_Explicit();
  TestDataSet_Structured2D();
  TestDataSet_Structured3D();
}
}

int UnitTestDataSetPermutation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestDataSet_Permutation, argc, argv);
}
