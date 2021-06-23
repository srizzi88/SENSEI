//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellSetExplicit.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace
{

using CellTag = svtkm::TopologyElementTagCell;
using PointTag = svtkm::TopologyElementTagPoint;

const svtkm::Id numberOfPoints = 11;

const svtkm::UInt8 g_shapes[] = { static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_HEXAHEDRON),
                                 static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_PYRAMID),
                                 static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_TETRA),
                                 static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_WEDGE) };
const svtkm::UInt8 g_shapes2[] = { g_shapes[1], g_shapes[2] };

const svtkm::Id g_offsets[] = { 0, 8, 13, 17, 23 };
const svtkm::Id g_offsets2[] = { 0, 5, 9 };

const svtkm::Id g_connectivity[] = { 0, 1, 5, 4,  3, 2, 6, 7, 1, 5, 6, 2,
                                    8, 5, 8, 10, 6, 4, 7, 9, 5, 6, 10 };
const svtkm::Id g_connectivity2[] = { 1, 5, 6, 2, 8, 5, 8, 10, 6 };

template <typename T, std::size_t Length>
svtkm::Id ArrayLength(const T (&)[Length])
{
  return static_cast<svtkm::Id>(Length);
}

// all points are part of atleast 1 cell
svtkm::cont::CellSetExplicit<> MakeTestCellSet1()
{
  svtkm::cont::CellSetExplicit<> cs;
  cs.Fill(numberOfPoints,
          svtkm::cont::make_ArrayHandle(g_shapes, ArrayLength(g_shapes)),
          svtkm::cont::make_ArrayHandle(g_connectivity, ArrayLength(g_connectivity)),
          svtkm::cont::make_ArrayHandle(g_offsets, ArrayLength(g_offsets)));
  return cs;
}

// some points are not part of any cell
svtkm::cont::CellSetExplicit<> MakeTestCellSet2()
{
  svtkm::cont::CellSetExplicit<> cs;
  cs.Fill(numberOfPoints,
          svtkm::cont::make_ArrayHandle(g_shapes2, ArrayLength(g_shapes2)),
          svtkm::cont::make_ArrayHandle(g_connectivity2, ArrayLength(g_connectivity2)),
          svtkm::cont::make_ArrayHandle(g_offsets2, ArrayLength(g_offsets2)));
  return cs;
}

struct WorkletPointToCell : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  SVTKM_EXEC void operator()(const PointIndicesType& pointIndices, svtkm::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint numCells);
  using ExecutionSignature = void(CellIndices, _2);
  using InputDomain = _1;

  template <typename CellIndicesType>
  SVTKM_EXEC void operator()(const CellIndicesType& cellIndices, svtkm::Id& numCells) const
  {
    numCells = cellIndices.GetNumberOfComponents();
  }
};

void TestCellSetExplicit()
{
  svtkm::cont::CellSetExplicit<> cellset;
  svtkm::cont::ArrayHandle<svtkm::Id> result;

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 1 (all points are part of atleast 1 cell): \n";
  cellset = MakeTestCellSet1();

  std::cout << "\tTesting PointToCell\n";
  svtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                   "result length not equal to number of cells");
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == cellset.GetNumberOfPointsInCell(i),
                     "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  svtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");

  svtkm::Id expected1[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == expected1[i], "incorrect result");
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 2 (some points are not part of any cell): \n";
  cellset = MakeTestCellSet2();

  std::cout << "\tTesting PointToCell\n";
  svtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                   "result length not equal to number of cells");
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == cellset.GetNumberOfPointsInCell(i),
                     "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  svtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  SVTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");

  svtkm::Id expected2[] = { 0, 1, 1, 0, 0, 2, 2, 0, 2, 0, 1 };
  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(
      result.GetPortalConstControl().Get(i) == expected2[i], "incorrect result at ", i);
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "General Testing: \n";

  std::cout << "\tTesting resource releasing in CellSetExplicit\n";
  cellset.ReleaseResourcesExecution();
  SVTKM_TEST_ASSERT(cellset.GetNumberOfCells() == ArrayLength(g_shapes) / 2,
                   "release execution resources should not change the number of cells");
  SVTKM_TEST_ASSERT(cellset.GetNumberOfPoints() == ArrayLength(expected2),
                   "release execution resources should not change the number of points");

  std::cout << "\tTesting CellToPoint table caching\n";
  cellset = MakeTestCellSet2();
  SVTKM_TEST_ASSERT(SVTKM_PASS_COMMAS(cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "PointToCell table missing.");
  SVTKM_TEST_ASSERT(SVTKM_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table exists before PrepareForInput.");

  // Test a raw PrepareForInput call:
  cellset.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{}, PointTag{}, CellTag{});

  SVTKM_TEST_ASSERT(SVTKM_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table missing after PrepareForInput.");

  cellset.ResetConnectivity(PointTag{}, CellTag{});
  SVTKM_TEST_ASSERT(SVTKM_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table exists after resetting.");

  // Test a PrepareForInput wrapped inside a dispatch (See #268)
  svtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);
  SVTKM_TEST_ASSERT(SVTKM_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table missing after CellToPoint worklet exec.");
}

} // anonymous namespace

int UnitTestCellSetExplicit(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellSetExplicit, argc, argv);
}
