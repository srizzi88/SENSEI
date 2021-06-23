//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_exec_celllocatorrectilineargrid_h
#define svtkm_exec_celllocatorrectilineargrid_h

#include <svtkm/Bounds.h>
#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>
#include <svtkm/VecFromPortalPermute.h>

#include <svtkm/cont/CellSetStructured.h>

#include <svtkm/exec/CellInside.h>
#include <svtkm/exec/CellLocator.h>
#include <svtkm/exec/ConnectivityStructured.h>
#include <svtkm/exec/ParametricCoordinates.h>

namespace svtkm
{

namespace exec
{

template <typename DeviceAdapter, svtkm::IdComponent dimensions>
class SVTKM_ALWAYS_EXPORT CellLocatorRectilinearGrid final : public svtkm::exec::CellLocator
{
private:
  using VisitType = svtkm::TopologyElementTagCell;
  using IncidentType = svtkm::TopologyElementTagPoint;
  using CellSetPortal = svtkm::exec::ConnectivityStructured<VisitType, IncidentType, dimensions>;

  using AxisHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using RectilinearType =
    svtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using AxisPortalType = typename AxisHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using RectilinearPortalType =
    typename RectilinearType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  SVTKM_CONT
  CellLocatorRectilinearGrid(const svtkm::Id planeSize,
                             const svtkm::Id rowSize,
                             const svtkm::cont::CellSetStructured<dimensions>& cellSet,
                             const RectilinearType& coords,
                             DeviceAdapter)
    : PlaneSize(planeSize)
    , RowSize(rowSize)
    , CellSet(cellSet.PrepareForInput(DeviceAdapter(), VisitType(), IncidentType()))
    , Coords(coords.PrepareForInput(DeviceAdapter()))
    , PointDimensions(cellSet.GetPointDimensions())
  {
    this->AxisPortals[0] = this->Coords.GetFirstPortal();
    this->MinPoint[0] = coords.GetPortalConstControl().GetFirstPortal().Get(0);
    this->MaxPoint[0] =
      coords.GetPortalConstControl().GetFirstPortal().Get(this->PointDimensions[0] - 1);

    this->AxisPortals[1] = this->Coords.GetSecondPortal();
    this->MinPoint[1] = coords.GetPortalConstControl().GetSecondPortal().Get(0);
    this->MaxPoint[1] =
      coords.GetPortalConstControl().GetSecondPortal().Get(this->PointDimensions[1] - 1);
    if (dimensions == 3)
    {
      this->AxisPortals[2] = this->Coords.GetThirdPortal();
      this->MinPoint[2] = coords.GetPortalConstControl().GetThirdPortal().Get(0);
      this->MaxPoint[2] =
        coords.GetPortalConstControl().GetThirdPortal().Get(this->PointDimensions[2] - 1);
    }
  }

  SVTKM_EXEC_CONT virtual ~CellLocatorRectilinearGrid() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC
  inline bool IsInside(const svtkm::Vec3f& point) const
  {
    bool inside = true;
    if (point[0] < this->MinPoint[0] || point[0] > this->MaxPoint[0])
      inside = false;
    if (point[1] < this->MinPoint[1] || point[1] > this->MaxPoint[1])
      inside = false;
    if (dimensions == 3)
    {
      if (point[2] < this->MinPoint[2] || point[2] > this->MaxPoint[2])
        inside = false;
    }
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
    for (svtkm::Int32 dim = 0; dim < dimensions; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        logicalCell[dim] = this->PointDimensions[dim] - 2;
        continue;
      }

      svtkm::Id minIndex = 0;
      svtkm::Id maxIndex = this->PointDimensions[dim] - 1;
      svtkm::FloatDefault minVal;
      svtkm::FloatDefault maxVal;
      minVal = this->AxisPortals[dim].Get(minIndex);
      maxVal = this->AxisPortals[dim].Get(maxIndex);
      while (maxIndex > minIndex + 1)
      {
        svtkm::Id midIndex = (minIndex + maxIndex) / 2;
        svtkm::FloatDefault midVal = this->AxisPortals[dim].Get(midIndex);
        if (point[dim] <= midVal)
        {
          maxIndex = midIndex;
          maxVal = midVal;
        }
        else
        {
          minIndex = midIndex;
          minVal = midVal;
        }
      }
      logicalCell[dim] = minIndex;
      parametric[dim] = (point[dim] - minVal) / (maxVal - minVal);
    }
    // Get the actual cellId, from the logical cell index of the cell
    cellId = logicalCell[2] * this->PlaneSize + logicalCell[1] * this->RowSize + logicalCell[0];
  }

private:
  svtkm::Id PlaneSize;
  svtkm::Id RowSize;

  CellSetPortal CellSet;
  RectilinearPortalType Coords;
  AxisPortalType AxisPortals[3];
  svtkm::Vec<svtkm::Id, dimensions> PointDimensions;
  svtkm::Vec3f MinPoint;
  svtkm::Vec3f MaxPoint;
};
} //namespace exec
} //namespace svtkm

#endif //svtkm_exec_celllocatorrectilineargrid_h
