//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingCellLocatorUniformGrid_h
#define svtk_m_cont_testing_TestingCellLocatorUniformGrid_h

#include <random>
#include <string>

#include <svtkm/cont/CellLocatorUniformGrid.h>
#include <svtkm/cont/DataSet.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/exec/CellLocatorUniformGrid.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

class LocatorWorklet : public svtkm::worklet::WorkletMapField
{
public:
  LocatorWorklet(svtkm::Bounds& bounds, svtkm::Id3& cellDims)
    : Bounds(bounds)
    , CellDims(cellDims)
  {
  }

  using ControlSignature =
    void(FieldIn pointIn, ExecObject locator, FieldOut cellId, FieldOut parametric, FieldOut match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  SVTKM_EXEC svtkm::Id CalculateCellId(const PointType& point) const
  {
    if (!Bounds.Contains(point))
      return -1;

    svtkm::Id3 logical;
    logical[0] = (point[0] == Bounds.X.Max)
      ? CellDims[0] - 1
      : static_cast<svtkm::Id>(svtkm::Floor((point[0] / Bounds.X.Length()) *
                                          static_cast<svtkm::FloatDefault>(CellDims[0])));
    logical[1] = (point[1] == Bounds.Y.Max)
      ? CellDims[1] - 1
      : static_cast<svtkm::Id>(svtkm::Floor((point[1] / Bounds.Y.Length()) *
                                          static_cast<svtkm::FloatDefault>(CellDims[1])));
    logical[2] = (point[2] == Bounds.Z.Max)
      ? CellDims[2] - 1
      : static_cast<svtkm::Id>(svtkm::Floor((point[2] / Bounds.Z.Length()) *
                                          static_cast<svtkm::FloatDefault>(CellDims[2])));

    return logical[2] * CellDims[0] * CellDims[1] + logical[1] * CellDims[0] + logical[0];
  }

  template <typename PointType, typename LocatorType>
  SVTKM_EXEC void operator()(const PointType& pointIn,
                            const LocatorType& locator,
                            svtkm::Id& cellId,
                            PointType& parametric,
                            bool& match) const
  {
    svtkm::Id calculated = CalculateCellId(pointIn);
    locator->FindCell(pointIn, cellId, parametric, (*this));
    match = (calculated == cellId);
  }

private:
  svtkm::Bounds Bounds;
  svtkm::Id3 CellDims;
};

template <typename DeviceAdapter>
class TestingCellLocatorUniformGrid
{
public:
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  void TestTest() const
  {
    svtkm::cont::DataSet dataset = svtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
    svtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
    svtkm::cont::DynamicCellSet cellSet = dataset.GetCellSet();

    svtkm::Bounds bounds = coords.GetBounds();
    std::cout << "X bounds : " << bounds.X.Min << " to " << bounds.X.Max << std::endl;
    std::cout << "Y bounds : " << bounds.Y.Min << " to " << bounds.Y.Max << std::endl;
    std::cout << "Z bounds : " << bounds.Z.Min << " to " << bounds.Z.Max << std::endl;

    using StructuredType = svtkm::cont::CellSetStructured<3>;
    svtkm::Id3 cellDims =
      cellSet.Cast<StructuredType>().GetSchedulingRange(svtkm::TopologyElementTagCell());
    std::cout << "Dimensions of dataset : " << cellDims << std::endl;

    svtkm::cont::CellLocatorUniformGrid locator;
    locator.SetCoordinates(coords);
    locator.SetCellSet(cellSet);

    locator.Update();

    // Generate some sample points.
    using PointType = svtkm::Vec3f;
    std::vector<PointType> pointsVec;
    std::default_random_engine dre;
    std::uniform_real_distribution<svtkm::Float32> inBounds(0.0f, 4.0f);
    for (size_t i = 0; i < 10; i++)
    {
      PointType point = svtkm::make_Vec(inBounds(dre), inBounds(dre), inBounds(dre));
      pointsVec.push_back(point);
    }
    std::uniform_real_distribution<svtkm::Float32> outBounds(4.0f, 5.0f);
    for (size_t i = 0; i < 5; i++)
    {
      PointType point = svtkm::make_Vec(outBounds(dre), outBounds(dre), outBounds(dre));
      pointsVec.push_back(point);
    }
    std::uniform_real_distribution<svtkm::Float32> outBounds2(-1.0f, 0.0f);
    for (size_t i = 0; i < 5; i++)
    {
      PointType point = svtkm::make_Vec(outBounds2(dre), outBounds2(dre), outBounds2(dre));
      pointsVec.push_back(point);
    }

    // Add points right on the boundary.
    pointsVec.push_back(svtkm::make_Vec(0, 0, 0));
    pointsVec.push_back(svtkm::make_Vec(4, 4, 4));
    pointsVec.push_back(svtkm::make_Vec(4, 0, 0));
    pointsVec.push_back(svtkm::make_Vec(0, 4, 0));
    pointsVec.push_back(svtkm::make_Vec(0, 0, 4));
    pointsVec.push_back(svtkm::make_Vec(4, 4, 0));
    pointsVec.push_back(svtkm::make_Vec(0, 4, 4));
    pointsVec.push_back(svtkm::make_Vec(4, 0, 4));

    svtkm::cont::ArrayHandle<PointType> points = svtkm::cont::make_ArrayHandle(pointsVec);
    // Query the points using the locators.
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
    svtkm::cont::ArrayHandle<PointType> parametric;
    svtkm::cont::ArrayHandle<bool> match;
    LocatorWorklet worklet(bounds, cellDims);
    svtkm::worklet::DispatcherMapField<LocatorWorklet> dispatcher(worklet);
    dispatcher.SetDevice(DeviceAdapter());
    dispatcher.Invoke(points, locator, cellIds, parametric, match);

    auto matchPortal = match.GetPortalConstControl();
    for (svtkm::Id index = 0; index < match.GetNumberOfValues(); index++)
    {
      SVTKM_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
    }
    std::cout << "Test finished successfully." << std::endl;
  }

  void operator()() const
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif
