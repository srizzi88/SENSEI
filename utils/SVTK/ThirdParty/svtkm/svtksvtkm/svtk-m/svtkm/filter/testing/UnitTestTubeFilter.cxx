//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/Tube.h>

namespace
{

void appendPts(svtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const svtkm::Vec3f& pt,
               std::vector<svtkm::Id>& ids)
{
  svtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}


void TestTubeFilters()
{
  using VecType = svtkm::Vec3f;

  svtkm::cont::DataSetBuilderExplicitIterative dsb;
  svtkm::cont::DataSetFieldAdd dsf;
  std::vector<svtkm::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 0, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 1, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  svtkm::cont::DataSet ds = dsb.Create();
  std::vector<svtkm::FloatDefault> ptVar, cellVar;

  //Polyline 1.
  ptVar.push_back(0);
  ptVar.push_back(1);
  ptVar.push_back(2);
  cellVar.push_back(100);
  cellVar.push_back(101);


  //Polyline 2.
  ptVar.push_back(10);
  ptVar.push_back(11);
  ptVar.push_back(12);
  cellVar.push_back(110);
  cellVar.push_back(111);

  dsf.AddPointField(ds, "pointVar", ptVar);
  dsf.AddCellField(ds, "cellVar", cellVar);

  svtkm::filter::Tube tubeFilter;
  tubeFilter.SetCapping(true);
  tubeFilter.SetNumberOfSides(3);
  tubeFilter.SetRadius(static_cast<svtkm::FloatDefault>(0.2));

  auto output = tubeFilter.Execute(ds);

  //Validate the result is correct.
  SVTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  svtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  SVTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 22, "Wrong number of coordinates");

  svtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  SVTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 36, "Wrong number of cells");

  //Validate the point field
  auto ptArr =
    output.GetField("pointVar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::FloatDefault>>();
  SVTKM_TEST_ASSERT(ptArr.GetNumberOfValues() == 22, "Wrong number of values in point field");

  std::vector<svtkm::FloatDefault> ptVals = { 0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  2,
                                             10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12 };
  auto portal = ptArr.GetPortalConstControl();
  for (svtkm::Id i = 0; i < 22; i++)
    SVTKM_TEST_ASSERT(portal.Get(i) == ptVals[static_cast<std::size_t>(i)],
                     "Wrong value for point field");


  //Validate the cell field
  auto cellArr =
    output.GetField("cellVar").GetData().Cast<svtkm::cont::ArrayHandle<svtkm::FloatDefault>>();
  SVTKM_TEST_ASSERT(cellArr.GetNumberOfValues() == 36, "Wrong number of values in cell field");
  std::vector<svtkm::FloatDefault> cellVals = { 100, 100, 100, 100, 100, 100, 101, 101, 101,
                                               101, 101, 101, 100, 100, 100, 101, 101, 101,
                                               110, 110, 110, 110, 110, 110, 111, 111, 111,
                                               111, 111, 111, 110, 110, 110, 111, 111, 111 };
  portal = cellArr.GetPortalConstControl();
  for (svtkm::Id i = 0; i < 36; i++)
    SVTKM_TEST_ASSERT(portal.Get(i) == cellVals[static_cast<std::size_t>(i)],
                     "Wrong value for cell field");
}
}

int UnitTestTubeFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestTubeFilters, argc, argv);
}
