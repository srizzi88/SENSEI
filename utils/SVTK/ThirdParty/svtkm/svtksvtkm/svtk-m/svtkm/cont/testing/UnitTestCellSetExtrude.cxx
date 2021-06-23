//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/cont/ArrayHandleExtrudeCoords.h>
#include <svtkm/cont/CellSetExtrude.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/filter/PointAverage.h>
#include <svtkm/filter/PointAverage.hxx>
#include <svtkm/filter/PolicyExtrude.h>

namespace
{
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<int> topology = { 0, 2, 1 };
std::vector<int> nextNode = { 0, 1, 2 };


struct CopyTopo : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldOutCell);
  typedef _2 ExecutionSignature(CellShape, PointIndices);
  template <typename T>
  T&& operator()(svtkm::CellShapeTagWedge, T&& t) const
  {
    return std::forward<T>(t);
  }
};

struct CopyReverseCellCount : public svtkm::worklet::WorkletVisitPointsWithCells
{
  typedef void ControlSignature(CellSetIn, FieldOutPoint);
  typedef _2 ExecutionSignature(CellShape, CellCount, CellIndices);

  template <typename T>
  svtkm::Int32 operator()(svtkm::CellShapeTagVertex shape, svtkm::IdComponent count, T&& t) const
  {
    if (shape.Id == svtkm::CELL_SHAPE_VERTEX)
    {
      bool valid = true;
      for (svtkm::IdComponent i = 0; i < count; ++i)
      {
        valid = valid && t[i] > 0;
      }
      return (valid && count == t.GetNumberOfComponents()) ? count : -1;
    }
    return -1;
  }
};

template <typename T, typename S>
void verify_topo(svtkm::cont::ArrayHandle<svtkm::Vec<T, 6>, S> const& handle, svtkm::Id expectedLen)
{
  auto portal = handle.GetPortalConstControl();
  SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == expectedLen, "topology portal size is incorrect");

  for (svtkm::Id i = 0; i < expectedLen - 1; ++i)
  {
    auto v = portal.Get(i);
    svtkm::Vec<svtkm::Id, 6> e;
    e[0] = (static_cast<svtkm::Id>(topology[0]) + (i * static_cast<svtkm::Id>(topology.size())));
    e[1] = (static_cast<svtkm::Id>(topology[1]) + (i * static_cast<svtkm::Id>(topology.size())));
    e[2] = (static_cast<svtkm::Id>(topology[2]) + (i * static_cast<svtkm::Id>(topology.size())));
    e[3] =
      (static_cast<svtkm::Id>(topology[0]) + ((i + 1) * static_cast<svtkm::Id>(topology.size())));
    e[4] =
      (static_cast<svtkm::Id>(topology[1]) + ((i + 1) * static_cast<svtkm::Id>(topology.size())));
    e[5] =
      (static_cast<svtkm::Id>(topology[2]) + ((i + 1) * static_cast<svtkm::Id>(topology.size())));
    std::cout << "v, e: " << v << ", " << e << "\n";
    SVTKM_TEST_ASSERT(test_equal(v, e), "incorrect conversion of topology to Cartesian space");
  }

  auto v = portal.Get(expectedLen - 1);
  svtkm::Vec<svtkm::Id, 6> e;
  e[0] = (static_cast<svtkm::Id>(topology[0]) +
          ((expectedLen - 1) * static_cast<svtkm::Id>(topology.size())));
  e[1] = (static_cast<svtkm::Id>(topology[1]) +
          ((expectedLen - 1) * static_cast<svtkm::Id>(topology.size())));
  e[2] = (static_cast<svtkm::Id>(topology[2]) +
          ((expectedLen - 1) * static_cast<svtkm::Id>(topology.size())));
  e[3] = (static_cast<svtkm::Id>(topology[0]));
  e[4] = (static_cast<svtkm::Id>(topology[1]));
  e[5] = (static_cast<svtkm::Id>(topology[2]));
  SVTKM_TEST_ASSERT(test_equal(v, e), "incorrect conversion of topology to Cartesian space");
}

int TestCellSetExtrude()
{
  const std::size_t numPlanes = 8;

  auto coords = svtkm::cont::make_ArrayHandleExtrudeCoords(points_rz, numPlanes, false);
  auto cells = svtkm::cont::make_CellSetExtrude(topology, coords, nextNode);
  SVTKM_TEST_ASSERT(cells.GetNumberOfPoints() == coords.GetNumberOfValues(),
                   "number of points don't match between cells and coordinates");

  // Verify the topology by copying it into another array
  {
    svtkm::cont::ArrayHandle<svtkm::Vec<int, 6>> output;
    svtkm::worklet::DispatcherMapTopology<CopyTopo> dispatcher;
    dispatcher.Invoke(cells, output);
    verify_topo(output, 8);
  }


  // Verify the reverse topology by copying the number of cells each point is
  // used by it into another array
  {
    svtkm::cont::ArrayHandle<int> output;
    svtkm::worklet::DispatcherMapTopology<CopyReverseCellCount> dispatcher;
    dispatcher.Invoke(cells, output);
    // verify_topo(output, 8);
  }

  //test a filter
  svtkm::cont::DataSet dataset;

  dataset.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", coords));
  dataset.SetCellSet(cells);

  // verify that a constant value point field can be accessed
  std::vector<float> pvalues(static_cast<size_t>(coords.GetNumberOfValues()), 42.0f);
  svtkm::cont::Field pfield(
    "pfield", svtkm::cont::Field::Association::POINTS, svtkm::cont::make_ArrayHandle(pvalues));
  dataset.AddField(pfield);

  // verify that a constant cell value can be accessed
  std::vector<float> cvalues(static_cast<size_t>(cells.GetNumberOfCells()), 42.0f);
  svtkm::cont::Field cfield =
    svtkm::cont::make_FieldCell("cfield", svtkm::cont::make_ArrayHandle(cvalues));
  dataset.AddField(cfield);

  svtkm::filter::PointAverage avg;
  try
  {
    avg.SetActiveField("cfield");
    auto result = avg.Execute(dataset, svtkm::filter::PolicyExtrude{});
    SVTKM_TEST_ASSERT(result.HasPointField("cfield"), "filter resulting dataset should be valid");
  }
  catch (const svtkm::cont::Error& err)
  {
    std::cout << err.GetMessage() << std::endl;
    SVTKM_TEST_ASSERT(false, "Filter execution threw an exception");
  }


  return 0;
}
}

int UnitTestCellSetExtrude(int argc, char* argv[])
{
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::Testing::Run(TestCellSetExtrude, argc, argv);
}
