//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_exec_celllocatoruniformgrid_h
#define svtkm_exec_celllocatoruniformgrid_h

#include <svtkm/Bounds.h>
#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>
#include <svtkm/VecFromPortalPermute.h>

#include <svtkm/cont/CellSetStructured.h>

#include <svtkm/exec/CellInside.h>
#include <svtkm/exec/CellLocator.h>
#include <svtkm/exec/ParametricCoordinates.h>

namespace svtkm
{

namespace exec
{

template <typename DeviceAdapter, svtkm::IdComponent dimensions>
class SVTKM_ALWAYS_EXPORT CellLocatorUniformGrid final : public svtkm::exec::CellLocator
{
private:
  using VisitType = svtkm::TopologyElementTagCell;
  using IncidentType = svtkm::TopologyElementTagPoint;
  using CellSetPortal = svtkm::exec::ConnectivityStructured<VisitType, IncidentType, dimensions>;
  using CoordsPortal = typename svtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

public:
  SVTKM_CONT
  CellLocatorUniformGrid(const svtkm::Id3 cellDims,
                         const svtkm::Id3 pointDims,
                         const svtkm::Vec3f origin,
                         const svtkm::Vec3f invSpacing,
                         const svtkm::Vec3f maxPoint,
                         const svtkm::cont::ArrayHandleVirtualCoordinates& coords,
                         DeviceAdapter)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Origin(origin)
    , InvSpacing(invSpacing)
    , MaxPoint(maxPoint)
    , Coords(coords.PrepareForInput(DeviceAdapter()))
  {
  }

  SVTKM_EXEC_CONT virtual ~CellLocatorUniformGrid() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC inline bool IsInside(const svtkm::Vec3f& point) const
  {
    bool inside = true;
    if (point[0] < this->Origin[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->Origin[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (point[2] < this->Origin[2] || point[2] > this->MaxPoint[2])
      inside = false;
    return inside;
  }

  SVTKM_EXEC
  void FindCell(const svtkm::Vec3f& point,
                svtkm::Id& cellId,
                svtkm::Vec3f& parametric,
                const svtkm::exec::FunctorBase& worklet) const override
  {
    (void)worklet; //suppress unused warning
    if (!this->IsInside(point))
    {
      cellId = -1;
      return;
    }
    // Get the Cell Id from the point.
    svtkm::Id3 logicalCell(0, 0, 0);

    svtkm::Vec3f temp;
    temp = point - this->Origin;
    temp = temp * this->InvSpacing;

    //make sure that if we border the upper edge, we sample the correct cell
    logicalCell = temp;
    if (logicalCell[0] == this->CellDims[0])
    {
      logicalCell[0]--;
    }
    if (logicalCell[1] == this->CellDims[1])
    {
      logicalCell[1]--;
    }
    if (logicalCell[2] == this->CellDims[2])
    {
      logicalCell[2]--;
    }
    if (dimensions == 2)
      logicalCell[2] = 0;
    cellId =
      (logicalCell[2] * this->CellDims[1] + logicalCell[1]) * this->CellDims[0] + logicalCell[0];
    parametric = temp - logicalCell;
  }

private:
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;
  svtkm::Vec3f Origin;
  svtkm::Vec3f InvSpacing;
  svtkm::Vec3f MaxPoint;
  CoordsPortal Coords;
};
}
}

#endif //svtkm_exec_celllocatoruniformgrid_h
