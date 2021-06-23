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
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/StreamSurface.h>

namespace
{
void appendPts(svtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const svtkm::Vec3f& pt,
               std::vector<svtkm::Id>& ids)
{
  svtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void TestSameNumPolylines()
{
  using VecType = svtkm::Vec3f;

  svtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<svtkm::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  svtkm::cont::DataSet ds = dsb.Create();
  svtkm::worklet::StreamSurface streamSurfaceWorklet;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> newPoints;
  svtkm::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  SVTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == ds.GetCoordinateSystem(0).GetNumberOfValues(),
                   "Wrong number of points in StreamSurface worklet");
  SVTKM_TEST_ASSERT(newCells.GetNumberOfCells() == 12,
                   "Wrong number of cells in StreamSurface worklet");
  /*
  svtkm::cont::DataSet ds2;
  ds2.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", newPoints));
  ds2.SetCellSet(newCells);
  svtkm::io::writer::SVTKDataSetWriter writer("srf.svtk");
  writer.WriteDataSet(ds2);
*/
}

void TestUnequalNumPolylines(int unequalType)
{
  using VecType = svtkm::Vec3f;

  svtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<svtkm::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  if (unequalType == 0)
  {
    appendPts(dsb, VecType(4, 0, 0), ids);
    appendPts(dsb, VecType(5, 0, 0), ids);
    appendPts(dsb, VecType(6, 0, 0), ids);
  }
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  if (unequalType == 1)
  {
    appendPts(dsb, VecType(4, 0, 1), ids);
    appendPts(dsb, VecType(5, 0, 1), ids);
    appendPts(dsb, VecType(6, 0, 1), ids);
  }
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  if (unequalType == 2)
  {
    appendPts(dsb, VecType(4, 0, 2), ids);
    appendPts(dsb, VecType(5, 0, 2), ids);
    appendPts(dsb, VecType(6, 0, 2), ids);
  }
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);

  svtkm::cont::DataSet ds = dsb.Create();
  svtkm::worklet::StreamSurface streamSurfaceWorklet;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> newPoints;
  svtkm::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  svtkm::Id numRequiredCells = (unequalType == 1 ? 18 : 15);

  SVTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == ds.GetCoordinateSystem(0).GetNumberOfValues(),
                   "Wrong number of points in StreamSurface worklet");
  SVTKM_TEST_ASSERT(newCells.GetNumberOfCells() == numRequiredCells,
                   "Wrong number of cells in StreamSurface worklet");

  /*
  svtkm::cont::DataSet ds2;
  ds2.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", newPoints));
  ds2.SetCellSet(newCells);
  svtkm::io::writer::SVTKDataSetWriter writer("srf.svtk");
  writer.WriteDataSet(ds2);
*/
}

void TestStreamSurface()
{
  std::cout << "Testing Stream Surface Worklet" << std::endl;
  TestSameNumPolylines();
  TestUnequalNumPolylines(0);
  TestUnequalNumPolylines(1);
  TestUnequalNumPolylines(2);
}
}

int UnitTestStreamSurface(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestStreamSurface, argc, argv);
}
