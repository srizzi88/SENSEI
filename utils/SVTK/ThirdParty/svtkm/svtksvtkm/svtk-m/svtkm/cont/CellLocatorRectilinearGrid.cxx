//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/CellLocatorRectilinearGrid.h>
#include <svtkm/cont/CellSetStructured.h>

#include <svtkm/exec/CellLocatorRectilinearGrid.h>

namespace svtkm
{
namespace cont
{

CellLocatorRectilinearGrid::CellLocatorRectilinearGrid() = default;

CellLocatorRectilinearGrid::~CellLocatorRectilinearGrid() = default;

using Structured2DType = svtkm::cont::CellSetStructured<2>;
using Structured3DType = svtkm::cont::CellSetStructured<3>;
using AxisHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
using RectilinearType = svtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;

void CellLocatorRectilinearGrid::Build()
{
  svtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  svtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<RectilinearType>())
    throw svtkm::cont::ErrorBadType("Coordinates are not rectilinear type.");

  if (cellSet.IsSameType(Structured2DType()))
  {
    this->Is3D = false;
    svtkm::Vec<svtkm::Id, 2> celldims =
      cellSet.Cast<Structured2DType>().GetSchedulingRange(svtkm::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else if (cellSet.IsSameType(Structured3DType()))
  {
    this->Is3D = true;
    svtkm::Vec<svtkm::Id, 3> celldims =
      cellSet.Cast<Structured3DType>().GetSchedulingRange(svtkm::TopologyElementTagCell());
    this->PlaneSize = celldims[0] * celldims[1];
    this->RowSize = celldims[0];
  }
  else
  {
    throw svtkm::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }
}

namespace
{

template <svtkm::IdComponent dimensions>
struct CellLocatorRectilinearGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  SVTKM_CONT bool operator()(DeviceAdapter,
                            svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = svtkm::exec::CellLocatorRectilinearGrid<DeviceAdapter, dimensions>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const svtkm::exec::CellLocator* CellLocatorRectilinearGrid::PrepareForExecution(
  svtkm::cont::DeviceAdapterId device) const
{
  bool success = false;
  if (this->Is3D)
  {
    success = svtkm::cont::TryExecuteOnDevice(
      device,
      CellLocatorRectilinearGridPrepareForExecutionFunctor<3>(),
      this->ExecutionObjectHandle,
      this->PlaneSize,
      this->RowSize,
      this->GetCellSet().template Cast<Structured3DType>(),
      this->GetCoordinates().GetData().template Cast<RectilinearType>());
  }
  else
  {
    success = svtkm::cont::TryExecuteOnDevice(
      device,
      CellLocatorRectilinearGridPrepareForExecutionFunctor<2>(),
      this->ExecutionObjectHandle,
      this->PlaneSize,
      this->RowSize,
      this->GetCellSet().template Cast<Structured2DType>(),
      this->GetCoordinates().GetData().template Cast<RectilinearType>());
  }
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorRectilinearGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}
} //namespace cont
} //namespace svtkm
