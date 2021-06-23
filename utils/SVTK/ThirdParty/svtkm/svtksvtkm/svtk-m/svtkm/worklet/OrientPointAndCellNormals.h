//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtkm_m_worklet_OrientPointAndCellNormals_h
#define svtkm_m_worklet_OrientPointAndCellNormals_h

#include <svtkm/Range.h>
#include <svtkm/Types.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleBitField.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayRangeCompute.h>
#include <svtkm/cont/BitField.h>
#include <svtkm/cont/Logging.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/MaskIndices.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientPointAndCellNormals
{
  static constexpr svtkm::Id INVALID_ID = -1;

  // Returns true if v1 and v2 are pointing in the same hemisphere.
  template <typename T>
  SVTKM_EXEC static bool SameDirection(const svtkm::Vec<T, 3>& v1, const svtkm::Vec<T, 3>& v2)
  {
    return svtkm::Dot(v1, v2) >= 0;
  }

  // Ensure that the normal is pointing in the same hemisphere as ref.
  // Returns true if normal is modified.
  template <typename T>
  SVTKM_EXEC static bool Align(svtkm::Vec<T, 3>& normal, const svtkm::Vec<T, 3>& ref)
  {
    if (!SameDirection(normal, ref))
    {
      normal = -normal;
      return true;
    }
    return false;
  }

public:
  // Locates starting points for BFS traversal of dataset by finding points
  // on the dataset boundaries. The normals for these points are corrected by
  // making them point outside of the dataset, and they are marked as both
  // active and visited.
  class WorkletMarkSourcePoints : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords,
                                  FieldInOut pointNormals,
                                  WholeArrayIn ranges,
                                  FieldOut activePoints,
                                  FieldOut visitedPoints);
    using ExecutionSignature =
      void(_1 coord, _2 pointNormal, _3 ranges, _4 activePoints, _5 visitedPoints);

    template <typename CoordT, typename NormalT, typename RangePortal>
    SVTKM_EXEC void operator()(const svtkm::Vec<CoordT, 3>& point,
                              svtkm::Vec<NormalT, 3>& pointNormal,
                              const RangePortal& ranges,
                              bool& isActive,
                              bool& isVisited) const
    {
      for (svtkm::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min)
        {
          svtkm::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(-1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
        else if (val >= range.Max)
        {
          svtkm::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
      }

      isActive = false;
      isVisited = false;
    }
  };

  // Mark each incident cell as active and visited.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _4(CellIndices cellIds, _2 activeCells, _3 visitedCells);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename CellList, typename ActiveCellsBitPortal, typename VisitedCellsBitPortal>
    SVTKM_EXEC bool operator()(const CellList& cellIds,
                              ActiveCellsBitPortal& activeCells,
                              VisitedCellsBitPortal& visitedCells) const
    {
      const svtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (svtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const svtkm::Id cellId = cellIds[c];
        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread owns this cell.
          activeCells.SetBitAtomic(cellId, true);
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the current cell's normals to an adjacent visited point's normal.
  class WorkletProcessCellNormals : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayIn pointNormals,
                                  WholeArrayInOut cellNormals,
                                  BitFieldIn visitedPoints);
    using ExecutionSignature = void(PointIndices pointIds,
                                    InputIndex cellId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedPoints);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename PointList,
              typename PointNormalsPortal,
              typename CellNormalsPortal,
              typename VisitedPointsBitPortal>
    SVTKM_EXEC void operator()(const PointList& pointIds,
                              const svtkm::Id cellId,
                              const PointNormalsPortal& pointNormals,
                              CellNormalsPortal& cellNormals,
                              const VisitedPointsBitPortal& visitedPoints) const
    {
      // Use the normal of a visited point as a reference:
      const svtkm::Id refPointId = [&]() -> svtkm::Id {
        const svtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
        for (svtkm::IdComponent p = 0; p < numPoints; ++p)
        {
          const svtkm::Id pointId = pointIds[p];
          if (visitedPoints.GetBit(pointId))
          {
            return pointId;
          }
        }

        return INVALID_ID;
      }();

      SVTKM_ASSERT("No reference point found." && refPointId != INVALID_ID);

      const auto refNormal = pointNormals.Get(refPointId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }
    }
  };

  // Mark each incident point as active and visited.
  // Marks the current cell as inactive.
  class WorkletMarkActivePoints : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activePoints,
                                  BitFieldInOut visitedPoints,
                                  FieldInOutCell activeCells);
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoint, _3 visitedPoint);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename PointList, typename ActivePointsBitPortal, typename VisitedPointsBitPortal>
    SVTKM_EXEC bool operator()(const PointList& pointIds,
                              ActivePointsBitPortal& activePoints,
                              VisitedPointsBitPortal& visitedPoints) const
    {
      const svtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
      for (svtkm::IdComponent p = 0; p < numPoints; ++p)
      {
        const svtkm::Id pointId = pointIds[p];
        if (!visitedPoints.OrBitAtomic(pointId, true))
        { // This thread owns this point.
          activePoints.SetBitAtomic(pointId, true);
        }
      }

      // Mark current cell as inactive:
      return false;
    }
  };

  // Align the current point's normals to an adjacent visited cell's normal.
  class WorkletProcessPointNormals : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayInOut pointNormals,
                                  WholeArrayIn cellNormals,
                                  BitFieldIn visitedCells);
    using ExecutionSignature = void(CellIndices cellIds,
                                    InputIndex pointId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedCells);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename CellList,
              typename CellNormalsPortal,
              typename PointNormalsPortal,
              typename VisitedCellsBitPortal>
    SVTKM_EXEC void operator()(const CellList& cellIds,
                              const svtkm::Id pointId,
                              PointNormalsPortal& pointNormals,
                              const CellNormalsPortal& cellNormals,
                              const VisitedCellsBitPortal& visitedCells) const
    {
      // Use the normal of a visited cell as a reference:
      const svtkm::Id refCellId = [&]() -> svtkm::Id {
        const svtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
        for (svtkm::IdComponent c = 0; c < numCells; ++c)
        {
          const svtkm::Id cellId = cellIds[c];
          if (visitedCells.GetBit(cellId))
          {
            return cellId;
          }
        }

        return INVALID_ID;
      }();

      SVTKM_ASSERT("No reference cell found." && refCellId != INVALID_ID);

      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = pointNormals.Get(pointId);
      if (Align(normal, refNormal))
      {
        pointNormals.Set(pointId, normal);
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  SVTKM_CONT static void Run(
    const CellSetType& cells,
    const svtkm::cont::ArrayHandle<svtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    svtkm::cont::ArrayHandle<svtkm::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals,
    svtkm::cont::ArrayHandle<svtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    using RangeType = svtkm::cont::ArrayHandle<svtkm::Range>;

    using MarkSourcePoints = svtkm::worklet::DispatcherMapField<WorkletMarkSourcePoints>;
    using MarkActiveCells = svtkm::worklet::DispatcherMapTopology<WorkletMarkActiveCells>;
    using ProcessCellNormals = svtkm::worklet::DispatcherMapTopology<WorkletProcessCellNormals>;
    using MarkActivePoints = svtkm::worklet::DispatcherMapTopology<WorkletMarkActivePoints>;
    using ProcessPointNormals = svtkm::worklet::DispatcherMapTopology<WorkletProcessPointNormals>;

    const svtkm::Id numCells = cells.GetNumberOfCells();

    SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
                   "OrientPointAndCellNormals worklet (%lld points, %lld cells)",
                   static_cast<svtkm::Int64>(coords.GetNumberOfValues()),
                   static_cast<svtkm::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    svtkm::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = svtkm::cont::make_ArrayHandleBitField(activePointBits);

    svtkm::cont::BitField activeCellBits;
    svtkm::cont::Algorithm::Fill(activeCellBits, false, numCells);
    auto activeCells = svtkm::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    svtkm::cont::BitField visitedPointBits; // Initialized by MarkSourcePoints
    auto visitedPoints = svtkm::cont::make_ArrayHandleBitField(visitedPointBits);

    svtkm::cont::BitField visitedCellBits;
    svtkm::cont::Algorithm::Fill(visitedCellBits, false, numCells);
    auto visitedCells = svtkm::cont::make_ArrayHandleBitField(visitedCellBits);

    svtkm::cont::ArrayHandle<svtkm::Id> mask; // Allocated as needed

    // 1) Compute range of coords.
    const RangeType ranges = svtkm::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary and align their normal to point out of the
    //    dataset:
    {
      MarkSourcePoints dispatcher;
      dispatcher.Invoke(coords, pointNormals, ranges, activePoints, visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 3) Mark unvisited cells adjacent to active points
      {
        svtkm::Id numActive = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "MarkActiveCells from " << numActive << " active points.");
        MarkActiveCells dispatcher{ svtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activeCellBits, visitedCellBits, activePoints);
      }

      svtkm::Id numActiveCells = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "Iteration " << iter << ": Traversal complete; no more cells");
        break;
      }

      SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActiveCells << " cell normals.");

      // 4) Correct normals for active cells.
      {
        ProcessCellNormals dispatcher{ svtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedPointBits);
      }

      // 5) Mark unvisited points adjacent to active cells
      {
        svtkm::Id numActive = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "MarkActivePoints from " << numActive << " active cells.");
        MarkActivePoints dispatcher{ svtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activePointBits, visitedPointBits, activeCells);
      }

      svtkm::Id numActivePoints =
        svtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);

      if (numActivePoints == 0)
      { // Done!
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "Iteration " << iter << ": Traversal complete; no more points");
        break;
      }

      SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActivePoints << " point normals.");

      // 4) Correct normals for active points.
      {
        ProcessPointNormals dispatcher{ svtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedCellBits);
      }
    }
  }
};
}
} // end namespace svtkm::worklet


#endif // svtkm_m_worklet_OrientPointAndCellNormals_h
