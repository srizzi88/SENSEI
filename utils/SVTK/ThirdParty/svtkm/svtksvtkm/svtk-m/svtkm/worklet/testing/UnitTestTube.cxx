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
#include <svtkm/worklet/Tube.h>

namespace
{
void appendPts(svtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const svtkm::Vec3f& pt,
               std::vector<svtkm::Id>& ids)
{
  svtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void createNonPoly(svtkm::cont::DataSetBuilderExplicitIterative& dsb)

{
  std::vector<svtkm::Id> ids;

  appendPts(dsb, svtkm::Vec3f(0, 0, 0), ids);
  appendPts(dsb, svtkm::Vec3f(1, 0, 0), ids);
  appendPts(dsb, svtkm::Vec3f(1, 1, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_TRIANGLE, ids);
}

svtkm::Id calcNumPoints(const std::size_t& numPtIds, const svtkm::Id& numSides, const bool& capEnds)
{
  //there are 'numSides' points for each polyline vertex
  //plus, 2 more for the center point of start and end caps.
  return static_cast<svtkm::Id>(numPtIds) * numSides + (capEnds ? 2 : 0);
}

svtkm::Id calcNumCells(const std::size_t& numPtIds, const svtkm::Id& numSides, const bool& capEnds)
{
  //Each line segment has numSides * 2 triangles.
  //plus, numSides triangles for each cap.
  return (2 * static_cast<svtkm::Id>(numPtIds - 1) * numSides) + (capEnds ? 2 * numSides : 0);
}

void TestTube(bool capEnds, svtkm::FloatDefault radius, svtkm::Id numSides, svtkm::Id insertNonPolyPos)
{
  using VecType = svtkm::Vec3f;

  svtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<svtkm::Id> ids;

  if (insertNonPolyPos == 0)
    createNonPoly(dsb);

  svtkm::Id reqNumPts = 0, reqNumCells = 0;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 1)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 0, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 2)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  appendPts(dsb, VecType(4, 0, 0), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 3)
    createNonPoly(dsb);

  //Add something a little more complicated...
  ids.clear();
  svtkm::FloatDefault x0 = 0;
  svtkm::FloatDefault x1 = static_cast<svtkm::FloatDefault>(6.28);
  svtkm::FloatDefault dx = static_cast<svtkm::FloatDefault>(0.05);
  for (svtkm::FloatDefault x = x0; x < x1; x += dx)
    appendPts(dsb, VecType(x, svtkm::Cos(x), svtkm::Sin(x) / 2), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 4)
    createNonPoly(dsb);

  //Finally, add a degenerate polyline: don't dance with the beast.
  ids.clear();
  appendPts(dsb, VecType(6, 6, 6), ids);
  dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
  //Should NOT produce a tubed polyline, so don't increment reqNumPts and reqNumCells.

  svtkm::cont::DataSet ds = dsb.Create();

  svtkm::worklet::Tube tubeWorklet(capEnds, numSides, radius);
  svtkm::cont::ArrayHandle<svtkm::Vec3f> newPoints;
  svtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0).GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Vec3f>>(),
                  ds.GetCellSet(),
                  newPoints,
                  newCells);

  SVTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == reqNumPts,
                   "Wrong number of points in Tube worklet");
  SVTKM_TEST_ASSERT(newCells.GetNumberOfCells() == reqNumCells, "Wrong cell shape in Tube worklet");
  SVTKM_TEST_ASSERT(newCells.GetCellShape(0) == svtkm::CELL_SHAPE_TRIANGLE,
                   "Wrong cell shape in Tube worklet");
}

void TestLinearPolylines()
{
  using VecType = svtkm::Vec3f;

  //Create a number of linear polylines along a set of directions.
  //We check that the tubes are all copacetic (proper number of cells, points),
  //and that the tube points all lie in the proper plane.
  //This will validate the code that computes the coordinate frame at each
  //vertex in the polyline. There are numeric checks to handle co-linear segments.

  std::vector<VecType> dirs;
  for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
      for (int k = -1; k <= 1; k++)
      {
        if (!i && !j && !k)
          continue;
        dirs.push_back(svtkm::Normal(VecType(static_cast<svtkm::FloatDefault>(i),
                                            static_cast<svtkm::FloatDefault>(j),
                                            static_cast<svtkm::FloatDefault>(k))));
      }

  bool capEnds = false;
  svtkm::Id numSides = 3;
  svtkm::FloatDefault radius = 1;
  for (auto& dir : dirs)
  {
    svtkm::cont::DataSetBuilderExplicitIterative dsb;
    std::vector<svtkm::Id> ids;

    VecType pt(0, 0, 0);
    for (int i = 0; i < 5; i++)
    {
      appendPts(dsb, pt, ids);
      pt = pt + dir;
    }

    dsb.AddCell(svtkm::CELL_SHAPE_POLY_LINE, ids);
    svtkm::cont::DataSet ds = dsb.Create();

    svtkm::Id reqNumPts = calcNumPoints(ids.size(), numSides, capEnds);
    svtkm::Id reqNumCells = calcNumCells(ids.size(), numSides, capEnds);

    svtkm::worklet::Tube tubeWorklet(capEnds, numSides, radius);
    svtkm::cont::ArrayHandle<svtkm::Vec3f> newPoints;
    svtkm::cont::CellSetSingleType<> newCells;
    tubeWorklet.Run(
      ds.GetCoordinateSystem(0).GetData().Cast<svtkm::cont::ArrayHandle<svtkm::Vec3f>>(),
      ds.GetCellSet(),
      newPoints,
      newCells);

    SVTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == reqNumPts,
                     "Wrong number of points in Tube worklet");
    SVTKM_TEST_ASSERT(newCells.GetNumberOfCells() == reqNumCells,
                     "Wrong cell shape in Tube worklet");
    SVTKM_TEST_ASSERT(newCells.GetCellShape(0) == svtkm::CELL_SHAPE_TRIANGLE,
                     "Wrong cell shape in Tube worklet");

    //Each of the 3 points should be in the plane defined by dir.
    auto portal = newPoints.GetPortalConstControl();
    for (svtkm::Id i = 0; i < newPoints.GetNumberOfValues(); i += 3)
    {
      auto p0 = portal.Get(i + 0);
      auto p1 = portal.Get(i + 1);
      auto p2 = portal.Get(i + 2);
      auto vec = svtkm::Normal(svtkm::Cross(p0 - p1, p0 - p2));
      svtkm::FloatDefault dp = svtkm::Abs(svtkm::Dot(vec, dir));
      SVTKM_TEST_ASSERT((1 - dp) <= svtkm::Epsilon<svtkm::FloatDefault>(),
                       "Tube points in wrong orientation");
    }
  }
}

void TestTubeWorklets()
{
  std::cout << "Testing Tube Worklet" << std::endl;

  std::vector<svtkm::Id> testNumSides = { 3, 4, 8, 13, 20 };
  std::vector<svtkm::FloatDefault> testRadii = { 0.01f, 0.05f, 0.10f };
  std::vector<int> insertNonPolylinePos = { -1, 0, 1, 2, 3, 4 };

  for (auto& i : insertNonPolylinePos)
    for (auto& n : testNumSides)
      for (auto& r : testRadii)
      {
        TestTube(false, r, n, i);
        TestTube(true, r, n, i);
      }

  TestLinearPolylines();
}
}

int UnitTestTube(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestTubeWorklets, argc, argv);
}
