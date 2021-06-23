//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particle_advection_cell_interpolation_helper
#define svtk_m_worklet_particle_advection_cell_interpolation_helper

#include <svtkm/CellShape.h>
#include <svtkm/Types.h>
#include <svtkm/VecVariable.h>

#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/exec/CellInterpolate.h>

/*
 * Interface to define the helper classes that can return mesh data
 * on a cell by cell basis.
 */
namespace svtkm
{
namespace exec
{

class CellInterpolationHelper : public svtkm::VirtualObjectBase
{
public:
  SVTKM_EXEC_CONT virtual ~CellInterpolationHelper() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC
  virtual void GetCellInfo(const svtkm::Id& cellId,
                           svtkm::UInt8& cellShape,
                           svtkm::IdComponent& numVerts,
                           svtkm::VecVariable<svtkm::Id, 8>& indices) const = 0;
};

class StructuredCellInterpolationHelper : public svtkm::exec::CellInterpolationHelper
{
public:
  StructuredCellInterpolationHelper() = default;

  SVTKM_CONT
  StructuredCellInterpolationHelper(svtkm::Id3 cellDims, svtkm::Id3 pointDims, bool is3D)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Is3D(is3D)
  {
  }

  SVTKM_EXEC
  void GetCellInfo(const svtkm::Id& cellId,
                   svtkm::UInt8& cellShape,
                   svtkm::IdComponent& numVerts,
                   svtkm::VecVariable<svtkm::Id, 8>& indices) const override
  {
    svtkm::Id3 logicalCellId;
    logicalCellId[0] = cellId % this->CellDims[0];
    logicalCellId[1] = (cellId / this->CellDims[0]) % this->CellDims[1];
    if (this->Is3D)
    {
      logicalCellId[2] = cellId / (this->CellDims[0] * this->CellDims[1]);
      indices.Append((logicalCellId[2] * this->PointDims[1] + logicalCellId[1]) *
                       this->PointDims[0] +
                     logicalCellId[0]);
      indices.Append(indices[0] + 1);
      indices.Append(indices[1] + this->PointDims[0]);
      indices.Append(indices[2] - 1);
      indices.Append(indices[0] + this->PointDims[0] * this->PointDims[1]);
      indices.Append(indices[4] + 1);
      indices.Append(indices[5] + this->PointDims[0]);
      indices.Append(indices[6] - 1);
      cellShape = static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_HEXAHEDRON);
      numVerts = 8;
    }
    else
    {
      indices.Append(logicalCellId[1] * this->PointDims[0] + logicalCellId[0]);
      indices.Append(indices[0] + 1);
      indices.Append(indices[1] + this->PointDims[0]);
      indices.Append(indices[2] - 1);
      cellShape = static_cast<svtkm::UInt8>(svtkm::CELL_SHAPE_QUAD);
      numVerts = 4;
    }
  }

private:
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;
  bool Is3D = true;
};

template <typename DeviceAdapter>
class SingleCellTypeInterpolationHelper : public svtkm::exec::CellInterpolationHelper
{
  using ConnType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  SingleCellTypeInterpolationHelper() = default;

  SVTKM_CONT
  SingleCellTypeInterpolationHelper(svtkm::UInt8 cellShape,
                                    svtkm::IdComponent pointsPerCell,
                                    const ConnType& connectivity)
    : CellShape(cellShape)
    , PointsPerCell(pointsPerCell)
    , Connectivity(connectivity.PrepareForInput(DeviceAdapter()))
  {
  }

  SVTKM_EXEC
  void GetCellInfo(const svtkm::Id& cellId,
                   svtkm::UInt8& cellShape,
                   svtkm::IdComponent& numVerts,
                   svtkm::VecVariable<svtkm::Id, 8>& indices) const override
  {
    cellShape = CellShape;
    numVerts = PointsPerCell;
    svtkm::Id n = static_cast<svtkm::Id>(PointsPerCell);
    svtkm::Id offset = cellId * n;

    for (svtkm::Id i = 0; i < n; i++)
      indices.Append(Connectivity.Get(offset + i));
  }

private:
  svtkm::UInt8 CellShape;
  svtkm::IdComponent PointsPerCell;
  ConnPortalType Connectivity;
};

template <typename DeviceAdapter>
class ExplicitCellInterpolationHelper : public svtkm::exec::CellInterpolationHelper
{
  using ShapeType = svtkm::cont::ArrayHandle<svtkm::UInt8>;
  using OffsetType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using ConnType = svtkm::cont::ArrayHandle<svtkm::Id>;

  using ShapePortalType = typename ShapeType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using OffsetPortalType = typename OffsetType::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using ConnPortalType = typename ConnType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  ExplicitCellInterpolationHelper() = default;

  SVTKM_CONT
  ExplicitCellInterpolationHelper(const ShapeType& shape,
                                  const OffsetType& offset,
                                  const ConnType& connectivity)
    : Shape(shape.PrepareForInput(DeviceAdapter()))
    , Offset(offset.PrepareForInput(DeviceAdapter()))
    , Connectivity(connectivity.PrepareForInput(DeviceAdapter()))
  {
  }

  SVTKM_EXEC
  void GetCellInfo(const svtkm::Id& cellId,
                   svtkm::UInt8& cellShape,
                   svtkm::IdComponent& numVerts,
                   svtkm::VecVariable<svtkm::Id, 8>& indices) const override
  {
    cellShape = this->Shape.Get(cellId);
    const svtkm::Id offset = this->Offset.Get(cellId);
    numVerts = static_cast<svtkm::IdComponent>(this->Offset.Get(cellId + 1) - offset);

    for (svtkm::IdComponent i = 0; i < numVerts; i++)
      indices.Append(this->Connectivity.Get(offset + i));
  }

private:
  ShapePortalType Shape;
  OffsetPortalType Offset;
  ConnPortalType Connectivity;
};

} // namespace exec

/*
 * Control side base object.
 */
namespace cont
{

class CellInterpolationHelper : public svtkm::cont::ExecutionObjectBase
{
public:
  using HandleType = svtkm::cont::VirtualObjectHandle<svtkm::exec::CellInterpolationHelper>;

  virtual ~CellInterpolationHelper() = default;

  SVTKM_CONT virtual const svtkm::exec::CellInterpolationHelper* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const = 0;
};

class StructuredCellInterpolationHelper : public svtkm::cont::CellInterpolationHelper
{
public:
  using Structured2DType = svtkm::cont::CellSetStructured<2>;
  using Structured3DType = svtkm::cont::CellSetStructured<3>;

  StructuredCellInterpolationHelper() = default;

  SVTKM_CONT
  StructuredCellInterpolationHelper(const svtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(Structured2DType()))
    {
      this->Is3D = false;
      svtkm::Id2 cellDims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(svtkm::TopologyElementTagCell());
      svtkm::Id2 pointDims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(svtkm::TopologyElementTagPoint());
      this->CellDims = svtkm::Id3(cellDims[0], cellDims[1], 0);
      this->PointDims = svtkm::Id3(pointDims[0], pointDims[1], 1);
    }
    else if (cellSet.IsSameType(Structured3DType()))
    {
      this->Is3D = true;
      this->CellDims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(svtkm::TopologyElementTagCell());
      this->PointDims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(svtkm::TopologyElementTagPoint());
    }
    else
      throw svtkm::cont::ErrorBadType("Cell set is not of type CellSetStructured");
  }

  SVTKM_CONT
  const svtkm::exec::CellInterpolationHelper* PrepareForExecution(
    svtkm::cont::DeviceAdapterId deviceId) const override
  {
    auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
    const bool valid = tracker.CanRunOn(deviceId);
    if (!valid)
    {
      throwFailedRuntimeDeviceTransfer("StructuredCellInterpolationHelper", deviceId);
    }

    using ExecutionType = svtkm::exec::StructuredCellInterpolationHelper;
    ExecutionType* execObject = new ExecutionType(this->CellDims, this->PointDims, this->Is3D);
    this->ExecHandle.Reset(execObject);

    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;
  bool Is3D = true;
  mutable HandleType ExecHandle;
};

class SingleCellTypeInterpolationHelper : public svtkm::cont::CellInterpolationHelper
{
public:
  using SingleExplicitType = svtkm::cont::CellSetSingleType<>;

  SingleCellTypeInterpolationHelper() = default;

  SVTKM_CONT
  SingleCellTypeInterpolationHelper(const svtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(SingleExplicitType()))
    {
      SingleExplicitType CellSet = cellSet.Cast<SingleExplicitType>();

      const auto cellShapes =
        CellSet.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
      const auto numIndices =
        CellSet.GetNumIndicesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

      CellShape = svtkm::cont::ArrayGetValue(0, cellShapes);
      PointsPerCell = svtkm::cont::ArrayGetValue(0, numIndices);
      Connectivity = CellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                  svtkm::TopologyElementTagPoint());
    }
    else
      throw svtkm::cont::ErrorBadType("Cell set is not of type CellSetSingleType");
  }

  struct SingleCellTypeFunctor
  {
    template <typename DeviceAdapter>
    SVTKM_CONT bool operator()(DeviceAdapter,
                              const svtkm::cont::SingleCellTypeInterpolationHelper& contInterpolator,
                              HandleType& execInterpolator) const
    {
      using ExecutionType = svtkm::exec::SingleCellTypeInterpolationHelper<DeviceAdapter>;
      ExecutionType* execObject = new ExecutionType(
        contInterpolator.CellShape, contInterpolator.PointsPerCell, contInterpolator.Connectivity);
      execInterpolator.Reset(execObject);
      return true;
    }
  };

  SVTKM_CONT
  const svtkm::exec::CellInterpolationHelper* PrepareForExecution(
    svtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success =
      svtkm::cont::TryExecuteOnDevice(deviceId, SingleCellTypeFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellTypeInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  svtkm::UInt8 CellShape;
  svtkm::IdComponent PointsPerCell;
  svtkm::cont::ArrayHandle<svtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

class ExplicitCellInterpolationHelper : public svtkm::cont::CellInterpolationHelper
{
public:
  ExplicitCellInterpolationHelper() = default;

  SVTKM_CONT
  ExplicitCellInterpolationHelper(const svtkm::cont::DynamicCellSet& cellSet)
  {
    if (cellSet.IsSameType(svtkm::cont::CellSetExplicit<>()))
    {
      svtkm::cont::CellSetExplicit<> CellSet = cellSet.Cast<svtkm::cont::CellSetExplicit<>>();
      Shape =
        CellSet.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
      Offset =
        CellSet.GetOffsetsArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
      Connectivity = CellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                  svtkm::TopologyElementTagPoint());
    }
    else
      throw svtkm::cont::ErrorBadType("Cell set is not of type CellSetExplicit");
  }

  struct ExplicitCellFunctor
  {
    template <typename DeviceAdapter>
    SVTKM_CONT bool operator()(DeviceAdapter,
                              const svtkm::cont::ExplicitCellInterpolationHelper& contInterpolator,
                              HandleType& execInterpolator) const
    {
      using ExecutionType = svtkm::exec::ExplicitCellInterpolationHelper<DeviceAdapter>;
      ExecutionType* execObject = new ExecutionType(
        contInterpolator.Shape, contInterpolator.Offset, contInterpolator.Connectivity);
      execInterpolator.Reset(execObject);
      return true;
    }
  };

  SVTKM_CONT
  const svtkm::exec::CellInterpolationHelper* PrepareForExecution(
    svtkm::cont::DeviceAdapterId deviceId) const override
  {
    const bool success =
      svtkm::cont::TryExecuteOnDevice(deviceId, ExplicitCellFunctor(), *this, this->ExecHandle);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("ExplicitCellInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId);
  }

private:
  svtkm::cont::ArrayHandle<svtkm::UInt8> Shape;
  svtkm::cont::ArrayHandle<svtkm::Id> Offset;
  svtkm::cont::ArrayHandle<svtkm::Id> Connectivity;
  mutable HandleType ExecHandle;
};

} //namespace cont
} //namespace svtkm

#endif //svtk_m_worklet_particle_advection_cell_interpolation_helper
