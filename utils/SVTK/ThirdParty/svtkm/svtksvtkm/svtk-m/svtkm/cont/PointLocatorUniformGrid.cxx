//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <svtkm/cont/PointLocatorUniformGrid.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

class BinPointsWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coord, FieldOut label);

  using ExecutionSignature = void(_1, _2);

  SVTKM_CONT
  BinPointsWorklet(svtkm::Vec3f min, svtkm::Vec3f max, svtkm::Id3 dims)
    : Min(min)
    , Dims(dims)
    , Dxdydz((max - Min) / Dims)
  {
  }

  template <typename CoordVecType, typename IdType>
  SVTKM_EXEC void operator()(const CoordVecType& coord, IdType& label) const
  {
    svtkm::Id3 ijk = (coord - Min) / Dxdydz;
    ijk = svtkm::Max(ijk, svtkm::Id3(0));
    ijk = svtkm::Min(ijk, this->Dims - svtkm::Id3(1));
    label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
  }

private:
  svtkm::Vec3f Min;
  svtkm::Id3 Dims;
  svtkm::Vec3f Dxdydz;
};

} // internal

void PointLocatorUniformGrid::Build()
{
  if (this->IsRangeInvalid())
  {
    this->Range = this->GetCoordinates().GetRange();
  }

  auto rmin = svtkm::make_Vec(static_cast<svtkm::FloatDefault>(this->Range[0].Min),
                             static_cast<svtkm::FloatDefault>(this->Range[1].Min),
                             static_cast<svtkm::FloatDefault>(this->Range[2].Min));
  auto rmax = svtkm::make_Vec(static_cast<svtkm::FloatDefault>(this->Range[0].Max),
                             static_cast<svtkm::FloatDefault>(this->Range[1].Max),
                             static_cast<svtkm::FloatDefault>(this->Range[2].Max));

  // generate unique id for each input point
  svtkm::cont::ArrayHandleCounting<svtkm::Id> pointCounting(
    0, 1, this->GetCoordinates().GetNumberOfValues());
  svtkm::cont::ArrayCopy(pointCounting, this->PointIds);

  using internal::BinPointsWorklet;

  // bin points into cells and give each of them the cell id.
  svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
  BinPointsWorklet cellIdWorklet(rmin, rmax, this->Dims);
  svtkm::worklet::DispatcherMapField<BinPointsWorklet> dispatchCellId(cellIdWorklet);
  dispatchCellId.Invoke(this->GetCoordinates(), cellIds);

  // Group points of the same cell together by sorting them according to the cell ids
  svtkm::cont::Algorithm::SortByKey(cellIds, this->PointIds);

  // for each cell, find the lower and upper bound of indices to the sorted point ids.
  svtkm::cont::ArrayHandleCounting<svtkm::Id> cell_ids_counting(
    0, 1, this->Dims[0] * this->Dims[1] * this->Dims[2]);
  svtkm::cont::Algorithm::UpperBounds(cellIds, cell_ids_counting, this->CellUpper);
  svtkm::cont::Algorithm::LowerBounds(cellIds, cell_ids_counting, this->CellLower);
}

struct PointLocatorUniformGrid::PrepareExecutionObjectFunctor
{
  template <typename DeviceAdapter>
  SVTKM_CONT bool operator()(DeviceAdapter,
                            const svtkm::cont::PointLocatorUniformGrid& self,
                            ExecutionObjectHandleType& handle) const
  {
    auto rmin = svtkm::make_Vec(static_cast<svtkm::FloatDefault>(self.Range[0].Min),
                               static_cast<svtkm::FloatDefault>(self.Range[1].Min),
                               static_cast<svtkm::FloatDefault>(self.Range[2].Min));
    auto rmax = svtkm::make_Vec(static_cast<svtkm::FloatDefault>(self.Range[0].Max),
                               static_cast<svtkm::FloatDefault>(self.Range[1].Max),
                               static_cast<svtkm::FloatDefault>(self.Range[2].Max));
    svtkm::exec::PointLocatorUniformGrid<DeviceAdapter>* h =
      new svtkm::exec::PointLocatorUniformGrid<DeviceAdapter>(
        rmin,
        rmax,
        self.Dims,
        self.GetCoordinates().GetData().PrepareForInput(DeviceAdapter()),
        self.PointIds.PrepareForInput(DeviceAdapter()),
        self.CellLower.PrepareForInput(DeviceAdapter()),
        self.CellUpper.PrepareForInput(DeviceAdapter()));
    handle.Reset(h);
    return true;
  }
};

SVTKM_CONT void PointLocatorUniformGrid::PrepareExecutionObject(
  ExecutionObjectHandleType& execObjHandle,
  svtkm::cont::DeviceAdapterId deviceId) const
{
  const bool success =
    svtkm::cont::TryExecuteOnDevice(deviceId, PrepareExecutionObjectFunctor(), *this, execObjHandle);
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("PointLocatorUniformGrid", deviceId);
  }
}
}
} // svtkm::cont
