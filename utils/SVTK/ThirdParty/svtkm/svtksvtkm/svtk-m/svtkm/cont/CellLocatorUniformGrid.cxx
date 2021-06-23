//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellLocatorUniformGrid.h>
#include <svtkm/cont/CellSetStructured.h>

#include <svtkm/exec/CellLocatorUniformGrid.h>

namespace svtkm
{
namespace cont
{
CellLocatorUniformGrid::CellLocatorUniformGrid() = default;

CellLocatorUniformGrid::~CellLocatorUniformGrid() = default;

using UniformType = svtkm::cont::ArrayHandleUniformPointCoordinates;
using Structured2DType = svtkm::cont::CellSetStructured<2>;
using Structured3DType = svtkm::cont::CellSetStructured<3>;

void CellLocatorUniformGrid::Build()
{
  svtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  svtkm::cont::DynamicCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw svtkm::cont::ErrorBadType("Coordinates are not uniform type.");

  if (cellSet.IsSameType(Structured2DType()))
  {
    this->Is3D = false;
    Structured2DType structuredCellSet = cellSet.Cast<Structured2DType>();
    svtkm::Id2 pointDims = structuredCellSet.GetSchedulingRange(svtkm::TopologyElementTagPoint());
    this->PointDims = svtkm::Id3(pointDims[0], pointDims[1], 1);
  }
  else if (cellSet.IsSameType(Structured3DType()))
  {
    this->Is3D = true;
    Structured3DType structuredCellSet = cellSet.Cast<Structured3DType>();
    this->PointDims = structuredCellSet.GetSchedulingRange(svtkm::TopologyElementTagPoint());
  }
  else
  {
    throw svtkm::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }

  UniformType uniformCoords = coords.GetData().Cast<UniformType>();
  this->Origin = uniformCoords.GetPortalConstControl().GetOrigin();

  svtkm::Vec3f spacing = uniformCoords.GetPortalConstControl().GetSpacing();
  svtkm::Vec3f unitLength;
  unitLength[0] = static_cast<svtkm::FloatDefault>(this->PointDims[0] - 1);
  unitLength[1] = static_cast<svtkm::FloatDefault>(this->PointDims[1] - 1);
  unitLength[2] = static_cast<svtkm::FloatDefault>(this->PointDims[2] - 1);

  this->MaxPoint = this->Origin + spacing * unitLength;
  this->InvSpacing[0] = 1.f / spacing[0];
  this->InvSpacing[1] = 1.f / spacing[1];
  this->InvSpacing[2] = 1.f / spacing[2];

  this->CellDims[0] = this->PointDims[0] - 1;
  this->CellDims[1] = this->PointDims[1] - 1;
  this->CellDims[2] = this->PointDims[2] - 1;
}

namespace
{
template <svtkm::IdComponent dimensions>
struct CellLocatorUniformGridPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename... Args>
  SVTKM_CONT bool operator()(DeviceAdapter,
                            svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator>& execLocator,
                            Args&&... args) const
  {
    using ExecutionType = svtkm::exec::CellLocatorUniformGrid<DeviceAdapter, dimensions>;
    ExecutionType* execObject = new ExecutionType(std::forward<Args>(args)..., DeviceAdapter());
    execLocator.Reset(execObject);
    return true;
  }
};
}

const svtkm::exec::CellLocator* CellLocatorUniformGrid::PrepareForExecution(
  svtkm::cont::DeviceAdapterId device) const
{
  bool success = true;
  if (this->Is3D)
  {
    success = svtkm::cont::TryExecuteOnDevice(device,
                                             CellLocatorUniformGridPrepareForExecutionFunctor<3>(),
                                             this->ExecutionObjectHandle,
                                             this->CellDims,
                                             this->PointDims,
                                             this->Origin,
                                             this->InvSpacing,
                                             this->MaxPoint,
                                             this->GetCoordinates().GetData());
  }
  else
  {
    success = svtkm::cont::TryExecuteOnDevice(device,
                                             CellLocatorUniformGridPrepareForExecutionFunctor<2>(),
                                             this->ExecutionObjectHandle,
                                             this->CellDims,
                                             this->PointDims,
                                             this->Origin,
                                             this->InvSpacing,
                                             this->MaxPoint,
                                             this->GetCoordinates().GetData());
  }
  if (!success)
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}

} //namespace cont
} //namespace svtkm
