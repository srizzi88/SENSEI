//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingCellLocatorRectilinearGrid_h
#define svtk_m_cont_testing_TestingCellLocatorRectilinearGrid_h

#include <random>
#include <string>

#include <svtkm/cont/CellLocatorRectilinearGrid.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/exec/CellLocatorRectilinearGrid.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

template <typename DeviceAdapter>
class LocatorWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using AxisHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using AxisPortalType = typename AxisHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using RectilinearType =
    svtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using RectilinearPortalType =
    typename RectilinearType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  LocatorWorklet(svtkm::Bounds& bounds, svtkm::Id3& dims, const RectilinearType& coords)
    : Bounds(bounds)
    , Dims(dims)
  {
    RectilinearPortalType coordsPortal = coords.PrepareForInput(DeviceAdapter());
    xAxis = coordsPortal.GetFirstPortal();
    yAxis = coordsPortal.GetSecondPortal();
    zAxis = coordsPortal.GetThirdPortal();
  }

  using ControlSignature =
    void(FieldIn pointIn, ExecObject locator, FieldOut cellId, FieldOut parametric, FieldOut match);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename PointType>
  SVTKM_EXEC svtkm::Id CalculateCellId(const PointType& point) const
  {
    if (!Bounds.Contains(point))
      return -1;
    svtkm::Id3 logical(-1, -1, -1);
    // Linear search in the coordinates.
    svtkm::Id index;
    /*Get floor X location*/
    if (point[0] == xAxis.Get(this->Dims[0] - 1))
      logical[0] = this->Dims[0] - 1;
    else
      for (index = 0; index < this->Dims[0] - 1; index++)
        if (xAxis.Get(index) <= point[0] && point[0] < xAxis.Get(index + 1))
        {
          logical[0] = index;
          break;
        }
    /*Get floor Y location*/
    if (point[1] == yAxis.Get(this->Dims[1] - 1))
      logical[1] = this->Dims[1] - 1;
    else
      for (index = 0; index < this->Dims[1] - 1; index++)
        if (yAxis.Get(index) <= point[1] && point[1] < yAxis.Get(index + 1))
        {
          logical[1] = index;
          break;
        }
    /*Get floor Z location*/
    if (point[2] == zAxis.Get(this->Dims[2] - 1))
      logical[2] = this->Dims[2] - 1;
    else
      for (index = 0; index < this->Dims[2] - 1; index++)
        if (zAxis.Get(index) <= point[2] && point[2] < zAxis.Get(index + 1))
        {
          logical[2] = index;
          break;
        }
    if (logical[0] == -1 || logical[1] == -1 || logical[2] == -1)
      return -1;
    return logical[2] * (Dims[0] - 1) * (Dims[1] - 1) + logical[1] * (Dims[0] - 1) + logical[0];
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
  svtkm::Id3 Dims;
  AxisPortalType xAxis;
  AxisPortalType yAxis;
  AxisPortalType zAxis;
};

template <typename DeviceAdapter>
class TestingCellLocatorRectilinearGrid
{
public:
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  void TestTest() const
  {
    svtkm::cont::DataSetBuilderRectilinear dsb;
    std::vector<svtkm::Float32> X(4), Y(3), Z(5);
    X[0] = 0.0f;
    X[1] = 1.0f;
    X[2] = 3.0f;
    X[3] = 4.0f;
    Y[0] = 0.0f;
    Y[1] = 1.0f;
    Y[2] = 2.0f;
    Z[0] = 0.0f;
    Z[1] = 1.0f;
    Z[2] = 3.0f;
    Z[3] = 5.0f;
    Z[4] = 6.0f;
    svtkm::cont::DataSet dataset = dsb.Create(X, Y, Z);

    using StructuredType = svtkm::cont::CellSetStructured<3>;
    using AxisHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
    using RectilinearType =
      svtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

    svtkm::cont::CoordinateSystem coords = dataset.GetCoordinateSystem();
    svtkm::cont::DynamicCellSet cellSet = dataset.GetCellSet();
    svtkm::Bounds bounds = coords.GetBounds();
    svtkm::Id3 dims =
      cellSet.Cast<StructuredType>().GetSchedulingRange(svtkm::TopologyElementTagPoint());

    // Generate some sample points.
    using PointType = svtkm::Vec3f;
    std::vector<PointType> pointsVec;
    std::default_random_engine dre;
    std::uniform_real_distribution<svtkm::Float32> xCoords(0.0f, 4.0f);
    std::uniform_real_distribution<svtkm::Float32> yCoords(0.0f, 2.0f);
    std::uniform_real_distribution<svtkm::Float32> zCoords(0.0f, 6.0f);
    for (size_t i = 0; i < 10; i++)
    {
      PointType point = svtkm::make_Vec(xCoords(dre), yCoords(dre), zCoords(dre));
      pointsVec.push_back(point);
    }

    svtkm::cont::ArrayHandle<PointType> points = svtkm::cont::make_ArrayHandle(pointsVec);

    // Initialize locator
    svtkm::cont::CellLocatorRectilinearGrid locator;
    locator.SetCoordinates(coords);
    locator.SetCellSet(cellSet);
    locator.Update();

    // Query the points using the locator.
    svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
    svtkm::cont::ArrayHandle<PointType> parametric;
    svtkm::cont::ArrayHandle<bool> match;
    LocatorWorklet<DeviceAdapter> worklet(
      bounds, dims, coords.GetData().template Cast<RectilinearType>());

    svtkm::worklet::DispatcherMapField<LocatorWorklet<DeviceAdapter>> dispatcher(worklet);
    dispatcher.SetDevice(DeviceAdapter());
    dispatcher.Invoke(points, locator, cellIds, parametric, match);

    auto matchPortal = match.GetPortalConstControl();
    for (svtkm::Id index = 0; index < match.GetNumberOfValues(); index++)
    {
      SVTKM_TEST_ASSERT(matchPortal.Get(index), "Points do not match");
    }
  }

  void operator()() const
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    this->TestTest();
  }
};

#endif
