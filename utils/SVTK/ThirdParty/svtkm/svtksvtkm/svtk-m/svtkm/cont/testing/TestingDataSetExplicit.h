//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_testing_TestingDataSetExplicit_h
#define svtk_m_cont_testing_TestingDataSetExplicit_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <set>

namespace svtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests DataSetExplicit
/// with the given DeviceAdapter
///
template <class DeviceAdapterTag>
class TestingDataSetExplicit
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

  static void TestDataSet_Explicit()
  {
    svtkm::cont::testing::MakeTestDataSet tds;
    svtkm::cont::DataSet ds = tds.Make3DExplicitDataSet0();

    SVTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");

    // test various field-getting methods and associations
    const svtkm::cont::Field& f1 = ds.GetField("pointvar");
    SVTKM_TEST_ASSERT(f1.GetAssociation() == svtkm::cont::Field::Association::POINTS,
                     "Association of 'pointvar' was not Association::POINTS");
    try
    {
      ds.GetCellField("cellvar");
    }
    catch (...)
    {
      SVTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
    }

    try
    {
      ds.GetPointField("cellvar");
      SVTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
    }
    catch (svtkm::cont::ErrorBadValue& error)
    {
      std::cout << "Caught expected error for association mismatch: " << std::endl
                << "    " << error.GetMessage() << std::endl;
    }

    SVTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                     "Incorrect number of coordinate systems");

    // test cell-to-point connectivity
    svtkm::cont::CellSetExplicit<> cellset;
    ds.GetCellSet().CopyTo(cellset);

    svtkm::Id connectivitySize = 7;
    svtkm::Id numPoints = 5;

    svtkm::UInt8 correctShapes[] = { 1, 1, 1, 1, 1 };
    svtkm::IdComponent correctNumIndices[] = { 1, 2, 2, 1, 1 };
    svtkm::Id correctConnectivity[] = { 0, 0, 1, 0, 1, 1, 1 };

    svtkm::cont::ArrayHandleConstant<svtkm::UInt8> shapes =
      cellset.GetShapesArray(svtkm::TopologyElementTagPoint(), svtkm::TopologyElementTagCell());
    auto numIndices =
      cellset.GetNumIndicesArray(svtkm::TopologyElementTagPoint(), svtkm::TopologyElementTagCell());
    svtkm::cont::ArrayHandle<svtkm::Id> conn =
      cellset.GetConnectivityArray(svtkm::TopologyElementTagPoint(), svtkm::TopologyElementTagCell());

    SVTKM_TEST_ASSERT(TestArrayHandle(shapes, correctShapes, numPoints), "Got incorrect shapes");
    SVTKM_TEST_ASSERT(TestArrayHandle(numIndices, correctNumIndices, numPoints),
                     "Got incorrect numIndices");

    // Some device adapters have unstable sorts, which may cause the order of
    // the indices for each point to be different but still correct. Iterate
    // over all the points and check the connectivity for each one.
    SVTKM_TEST_ASSERT(conn.GetNumberOfValues() == connectivitySize,
                     "Connectivity array wrong size.");
    svtkm::Id connectivityIndex = 0;
    for (svtkm::Id pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      svtkm::IdComponent numIncidentCells = correctNumIndices[pointIndex];
      std::set<svtkm::Id> correctIncidentCells;
      for (svtkm::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
      {
        correctIncidentCells.insert(correctConnectivity[connectivityIndex + cellIndex]);
      }
      for (svtkm::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
      {
        svtkm::Id expectedCell = conn.GetPortalConstControl().Get(connectivityIndex + cellIndex);
        std::set<svtkm::Id>::iterator foundCell = correctIncidentCells.find(expectedCell);
        SVTKM_TEST_ASSERT(foundCell != correctIncidentCells.end(),
                         "An incident cell in the connectivity list is wrong or repeated.");
        correctIncidentCells.erase(foundCell);
      }
      connectivityIndex += numIncidentCells;
    }

    //verify that GetIndices works properly
    svtkm::Id expectedPointIds[4] = { 2, 1, 3, 4 };
    svtkm::Id4 retrievedPointIds;
    cellset.GetIndices(1, retrievedPointIds);
    for (svtkm::IdComponent i = 0; i < 4; i++)
    {
      SVTKM_TEST_ASSERT(retrievedPointIds[i] == expectedPointIds[i],
                       "Incorrect point ID for quad cell");
    }
  }

  struct TestAll
  {
    SVTKM_CONT void operator()() const { TestingDataSetExplicit::TestDataSet_Explicit(); }
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

#endif // svtk_m_cont_testing_TestingDataSetExplicit_h
