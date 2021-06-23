//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_testing_TestingDataSetSingleType_h
#define svtk_m_cont_testing_TestingDataSetSingleType_h

#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/worklet/CellAverage.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace svtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests DataSetSingleType
/// with the given DeviceAdapter
///
template <class DeviceAdapterTag>
class TestingDataSetSingleType
{
private:
  template <typename T, typename Storage>
  static bool TestArrayHandle(const svtkm::cont::ArrayHandle<T, Storage>& ah,
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

  static inline svtkm::cont::DataSet make_SingleTypeDataSet()
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

    svtkm::cont::DataSetFieldAdd fieldAdder;
    fieldAdder.AddPointField(ds, "pointvar", vars, nVerts);

    return ds;
  }

  static void TestDataSet_SingleType()
  {
    svtkm::cont::DataSet dataSet = make_SingleTypeDataSet();

    //verify that we can get a CellSetSingleType from a dataset
    svtkm::cont::CellSetSingleType<> cellset;
    dataSet.GetCellSet().CopyTo(cellset);

    //verify that the point to cell connectivity types are correct
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8> shapesPointToCell =
      cellset.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
    svtkm::cont::ArrayHandle<svtkm::Id> connPointToCell =
      cellset.GetConnectivityArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

    SVTKM_TEST_ASSERT(shapesPointToCell.GetNumberOfValues() == 3, "Wrong number of shapes");
    SVTKM_TEST_ASSERT(connPointToCell.GetNumberOfValues() == 9, "Wrong connectivity length");

    //verify that the cell to point connectivity types are correct
    //note the handle storage types differ compared to point to cell
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8> shapesCellToPoint =
      cellset.GetShapesArray(svtkm::TopologyElementTagPoint(), svtkm::TopologyElementTagCell());
    svtkm::cont::ArrayHandle<svtkm::Id> connCellToPoint =
      cellset.GetConnectivityArray(svtkm::TopologyElementTagPoint(), svtkm::TopologyElementTagCell());

    SVTKM_TEST_ASSERT(shapesCellToPoint.GetNumberOfValues() == 5, "Wrong number of shapes");
    SVTKM_TEST_ASSERT(connCellToPoint.GetNumberOfValues() == 9, "Wrong connectivity length");

    //run a basic for-each topology algorithm on this
    svtkm::cont::ArrayHandle<svtkm::Float32> result;
    svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellAverage> dispatcher;
    dispatcher.SetDevice(DeviceAdapterTag());
    dispatcher.Invoke(cellset, dataSet.GetField("pointvar"), result);

    svtkm::Float32 expected[3] = { 20.1333f, 30.1667f, 40.2333f };
    for (int i = 0; i < 3; ++i)
    {
      SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(i), expected[i]),
                       "Wrong result for CellAverage worklet on explicit single type cellset data");
    }
  }

  struct TestAll
  {
    SVTKM_CONT void operator()() const { TestingDataSetSingleType::TestDataSet_SingleType(); }
  };

public:
  static SVTKM_CONT int Run(int argc, char* argv[])
  {
    return svtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif // svtk_m_cont_testing_TestingDataSetSingleType_h
