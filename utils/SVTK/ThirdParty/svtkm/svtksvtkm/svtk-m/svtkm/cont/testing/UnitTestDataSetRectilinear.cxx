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

#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/exec/ConnectivityStructured.h>

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

static void TwoDimRectilinearTest();
static void ThreeDimRectilinearTest();

void TestDataSet_Rectilinear()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Rectilinear--" << std::endl << std::endl;

  TwoDimRectilinearTest();
  ThreeDimRectilinearTest();
}

static void TwoDimRectilinearTest()
{
  std::cout << "2D Rectilinear data set" << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;

  svtkm::cont::DataSet dataSet = testDataSet.Make2DRectilinearDataSet0();

  svtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  SVTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 2, "Incorrect number of fields");
  SVTKM_TEST_ASSERT(dataSet.GetNumberOfCoordinateSystems() == 1,
                   "Incorrect number of coordinate systems");
  SVTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Incorrect number of points");
  SVTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Incorrect number of cells");

  // test various field-getting methods and associations
  try
  {
    dataSet.GetCellField("cellvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    dataSet.GetPointField("pointvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  for (svtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    SVTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 4,
                     "Incorrect number of cell indices");
    svtkm::IdComponent shape = cellSet.GetCellShape();
    SVTKM_TEST_ASSERT(shape == svtkm::CELL_SHAPE_QUAD, "Incorrect element type.");
  }

  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 2>
    pointToCell = cellSet.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                                          svtkm::TopologyElementTagCell(),
                                          svtkm::TopologyElementTagPoint());
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell, 2>
    cellToPoint = cellSet.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                                          svtkm::TopologyElementTagPoint(),
                                          svtkm::TopologyElementTagCell());

  svtkm::Id cells[2][4] = { { 0, 1, 4, 3 }, { 1, 2, 5, 4 } };
  for (svtkm::Id cellIndex = 0; cellIndex < 2; cellIndex++)
  {
    svtkm::Id4 pointIds = pointToCell.GetIndices(pointToCell.FlatToLogicalToIndex(cellIndex));
    for (svtkm::IdComponent localPointIndex = 0; localPointIndex < 4; localPointIndex++)
    {
      SVTKM_TEST_ASSERT(pointIds[localPointIndex] == cells[cellIndex][localPointIndex],
                       "Incorrect point ID for cell");
    }
  }

  svtkm::Id expectedCellIds[6][4] = { { 0, -1, -1, -1 }, { 0, 1, -1, -1 }, { 1, -1, -1, -1 },
                                     { 0, -1, -1, -1 }, { 0, 1, -1, -1 }, { 1, -1, -1, -1 } };

  for (svtkm::Id pointIndex = 0; pointIndex < 6; pointIndex++)
  {
    svtkm::VecVariable<svtkm::Id, 4> retrievedCellIds =
      cellToPoint.GetIndices(cellToPoint.FlatToLogicalToIndex(pointIndex));
    SVTKM_TEST_ASSERT(retrievedCellIds.GetNumberOfComponents() <= 4,
                     "Got wrong number of cell ids.");
    for (svtkm::IdComponent cellIndex = 0; cellIndex < retrievedCellIds.GetNumberOfComponents();
         cellIndex++)
    {
      SVTKM_TEST_ASSERT(retrievedCellIds[cellIndex] == expectedCellIds[pointIndex][cellIndex],
                       "Incorrect cell ID for point");
    }
  }
}

static void ThreeDimRectilinearTest()
{
  std::cout << "3D Rectilinear data set" << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;

  svtkm::cont::DataSet dataSet = testDataSet.Make3DRectilinearDataSet0();

  svtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  SVTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 2, "Incorrect number of fields");

  SVTKM_TEST_ASSERT(dataSet.GetNumberOfCoordinateSystems() == 1,
                   "Incorrect number of coordinate systems");

  SVTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 18, "Incorrect number of points");

  SVTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 4, "Incorrect number of cells");

  try
  {
    dataSet.GetCellField("cellvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    dataSet.GetPointField("pointvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  for (svtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    SVTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 8,
                     "Incorrect number of cell indices");
    svtkm::IdComponent shape = cellSet.GetCellShape();
    SVTKM_TEST_ASSERT(shape == svtkm::CELL_SHAPE_HEXAHEDRON, "Incorrect element type.");
  }

  //Test regular connectivity.
  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint, 3>
    pointToCell = cellSet.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                                          svtkm::TopologyElementTagCell(),
                                          svtkm::TopologyElementTagPoint());
  svtkm::Id expectedPointIds[8] = { 0, 1, 4, 3, 6, 7, 10, 9 };
  svtkm::Vec<svtkm::Id, 8> retrievedPointIds = pointToCell.GetIndices(svtkm::Id3(0));
  for (svtkm::IdComponent localPointIndex = 0; localPointIndex < 8; localPointIndex++)
  {
    SVTKM_TEST_ASSERT(retrievedPointIds[localPointIndex] == expectedPointIds[localPointIndex],
                     "Incorrect point ID for cell");
  }

  svtkm::exec::ConnectivityStructured<svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell, 3>
    cellToPoint = cellSet.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial(),
                                          svtkm::TopologyElementTagPoint(),
                                          svtkm::TopologyElementTagCell());
  svtkm::Id retrievedCellIds[6] = { 0, -1, -1, -1, -1, -1 };
  svtkm::VecVariable<svtkm::Id, 6> expectedCellIds = cellToPoint.GetIndices(svtkm::Id3(0));
  SVTKM_TEST_ASSERT(expectedCellIds.GetNumberOfComponents() <= 6,
                   "Got unexpected number of cell ids");
  for (svtkm::IdComponent localPointIndex = 0;
       localPointIndex < expectedCellIds.GetNumberOfComponents();
       localPointIndex++)
  {
    SVTKM_TEST_ASSERT(expectedCellIds[localPointIndex] == retrievedCellIds[localPointIndex],
                     "Incorrect cell ID for point");
  }
}

int UnitTestDataSetRectilinear(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestDataSet_Rectilinear, argc, argv);
}
