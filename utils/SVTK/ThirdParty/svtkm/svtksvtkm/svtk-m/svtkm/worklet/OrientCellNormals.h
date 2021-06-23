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
#ifndef svtkm_m_worklet_OrientCellNormals_h
#define svtkm_m_worklet_OrientCellNormals_h

#include <svtkm/Range.h>
#include <svtkm/Types.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleBitField.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayRangeCompute.h>
#include <svtkm/cont/BitField.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/Logging.h>

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
class OrientCellNormals
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
  // on the dataset boundaries. These points are marked as active.
  // Initializes the ActivePoints array.
  class WorkletMarkSourcePoints : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords, WholeArrayIn ranges, FieldOut activePoints);
    using ExecutionSignature = _3(_1 coord, _2 ranges);

    template <typename CoordT, typename RangePortal>
    SVTKM_EXEC bool operator()(const svtkm::Vec<CoordT, 3>& point, const RangePortal& ranges) const
    {
      bool isActive = false;
      for (svtkm::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min || val >= range.Max)
        {
          isActive = true;
        }
      }
      return isActive;
    }
  };

  // For each of the source points, determine the boundaries it lies on. Align
  // each incident cell's normal to point out of the boundary, marking each cell
  // as both visited and active.
  // Clears the active flags for points, and marks the current point as visited.
  class WorkletProcessSourceCells : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  FieldInPoint coords,
                                  WholeArrayIn ranges,
                                  WholeArrayInOut cellNormals,
                                  // InOut for preserve data on masked indices
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints,
                                  FieldInOutPoint visitedPoints);
    using ExecutionSignature = void(CellIndices cellIds,
                                    _2 coords,
                                    _3 ranges,
                                    _4 cellNormals,
                                    _5 activeCells,
                                    _6 visitedCells,
                                    _7 activePoints,
                                    _8 visitedPoints);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename CellList,
              typename CoordComp,
              typename RangePortal,
              typename CellNormalPortal,
              typename ActiveCellsBitPortal,
              typename VisitedCellsBitPortal>
    SVTKM_EXEC void operator()(const CellList& cellIds,
                              const svtkm::Vec<CoordComp, 3>& coord,
                              const RangePortal& ranges,
                              CellNormalPortal& cellNormals,
                              ActiveCellsBitPortal& activeCells,
                              VisitedCellsBitPortal& visitedCells,
                              bool& pointIsActive,
                              bool& pointIsVisited) const
    {
      using NormalType = typename CellNormalPortal::ValueType;
      SVTKM_STATIC_ASSERT_MSG(svtkm::VecTraits<NormalType>::NUM_COMPONENTS == 3,
                             "Cell Normals expected to have 3 components.");
      using NormalCompType = typename NormalType::ComponentType;

      // Find the vector that points out of the dataset from the current point:
      const NormalType refNormal = [&]() -> NormalType {
        NormalType normal{ NormalCompType{ 0 } };
        NormalCompType numNormals{ 0 };
        for (svtkm::IdComponent dim = 0; dim < 3; ++dim)
        {
          const auto range = ranges.Get(dim);
          const auto val = static_cast<decltype(range.Min)>(coord[dim]);
          if (val <= range.Min)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ -1 };
            normal += compNormal;
            ++numNormals;
          }
          else if (val >= range.Max)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ 1 };
            normal += compNormal;
            ++numNormals;
          }
        }

        SVTKM_ASSERT("Source point is not on a boundary?" && numNormals > 0.5);
        return normal / numNormals;
      }();

      // Align all cell normals to the reference, marking them as active and
      // visited.
      const svtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (svtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const svtkm::Id cellId = cellIds[c];

        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread is the first to touch this cell.
          activeCells.SetBitAtomic(cellId, true);

          NormalType cellNormal = cellNormals.Get(cellId);
          if (Align(cellNormal, refNormal))
          {
            cellNormals.Set(cellId, cellNormal);
          }
        }
      }

      // Mark current point as inactive but visited:
      pointIsActive = false;
      pointIsVisited = true;
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
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoints, _3 visitedPoints);

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

  // Mark each incident cell as active, setting a visited neighbor
  // cell as its reference for alignment.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayOut refCells,
                                  BitFieldInOut activeCells,
                                  BitFieldIn visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _5(CellIndices cellIds,
                                  _2 refCells,
                                  _3 activeCells,
                                  _4 visitedCells);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename CellList,
              typename RefCellPortal,
              typename ActiveCellBitPortal,
              typename VisitedCellBitPortal>
    SVTKM_EXEC bool operator()(const CellList& cellIds,
                              RefCellPortal& refCells,
                              ActiveCellBitPortal& activeCells,
                              const VisitedCellBitPortal& visitedCells) const
    {
      // One of the cells must be marked visited already. Find it and use it as
      // an alignment reference for the others:
      const svtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      const svtkm::Id refCellId = [&]() -> svtkm::Id {
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

      for (svtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const svtkm::Id cellId = cellIds[c];
        if (!visitedCells.GetBit(cellId))
        {
          if (!activeCells.OrBitAtomic(cellId, true))
          { // This thread owns this cell.
            refCells.Set(cellId, refCellId);
          }
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the normal of each active cell, to its reference cell normal.
  // The cell is marked visited.
  class WorkletProcessCellNormals : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn refCells,
                                  WholeArrayInOut cellNormals,
                                  FieldInOut visitedCells);
    using ExecutionSignature = _3(InputIndex cellId, _1 refCellId, _2 cellNormals);

    using MaskType = svtkm::worklet::MaskIndices;

    template <typename CellNormalsPortal>
    SVTKM_EXEC bool operator()(const svtkm::Id cellId,
                              const svtkm::Id refCellId,
                              CellNormalsPortal& cellNormals) const
    {
      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }

      // Mark cell as visited:
      return true;
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  SVTKM_CONT static void Run(
    const CellSetType& cells,
    const svtkm::cont::ArrayHandle<svtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    svtkm::cont::ArrayHandle<svtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    using RangeType = svtkm::cont::ArrayHandle<svtkm::Range>;

    const svtkm::Id numPoints = coords.GetNumberOfValues();
    const svtkm::Id numCells = cells.GetNumberOfCells();

    SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
                   "OrientCellNormals worklet (%lld points, %lld cells)",
                   static_cast<svtkm::Int64>(numPoints),
                   static_cast<svtkm::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    svtkm::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = svtkm::cont::make_ArrayHandleBitField(activePointBits);

    svtkm::cont::BitField activeCellBits;
    svtkm::cont::Algorithm::Fill(activeCellBits, false, numCells);
    auto activeCells = svtkm::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    svtkm::cont::BitField visitedPointBits;
    svtkm::cont::Algorithm::Fill(visitedPointBits, false, numPoints);
    auto visitedPoints = svtkm::cont::make_ArrayHandleBitField(visitedPointBits);

    svtkm::cont::BitField visitedCellBits;
    svtkm::cont::Algorithm::Fill(visitedCellBits, false, numCells);
    auto visitedCells = svtkm::cont::make_ArrayHandleBitField(visitedCellBits);

    svtkm::cont::Invoker invoke;
    svtkm::cont::ArrayHandle<svtkm::Id> mask; // Allocated as needed

    // For each cell, store a reference alignment cell.
    svtkm::cont::ArrayHandle<svtkm::Id> refCells;
    {
      svtkm::cont::Algorithm::Copy(
        svtkm::cont::make_ArrayHandleConstant<svtkm::Id>(INVALID_ID, numCells), refCells);
    }

    // 1) Compute range of coords.
    const RangeType ranges = svtkm::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary, since their normal alignment direction
    //    is known.
    invoke(WorkletMarkSourcePoints{}, coords, ranges, activePoints);

    // 3) For each source point, align the normals of the adjacent cells.
    {
      svtkm::Id numActive = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
      (void)numActive;
      SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                 "ProcessSourceCells from " << numActive << " source points.");
      invoke(WorkletProcessSourceCells{},
             svtkm::worklet::MaskIndices{ mask },
             cells,
             coords,
             ranges,
             cellNormals,
             activeCellBits,
             visitedCellBits,
             activePoints,
             visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 4) Mark unvisited points adjacent to active cells
      {
        svtkm::Id numActive = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "MarkActivePoints from " << numActive << " active cells.");
        invoke(WorkletMarkActivePoints{},
               svtkm::worklet::MaskIndices{ mask },
               cells,
               activePointBits,
               visitedPointBits,
               activeCells);
      }

      // 5) Mark unvisited cells adjacent to active points
      {
        svtkm::Id numActive = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                   "MarkActiveCells from " << numActive << " active points.");
        invoke(WorkletMarkActiveCells{},
               svtkm::worklet::MaskIndices{ mask },
               cells,
               refCells,
               activeCellBits,
               visitedCellBits,
               activePoints);
      }

      svtkm::Id numActiveCells = svtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        SVTKM_LOG_S(svtkm::cont::LogLevel::Perf, "Iteration " << iter << ": Traversal complete.");
        break;
      }

      SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActiveCells << " normals.");

      // 5) Correct normals for active cells.
      {
        invoke(WorkletProcessCellNormals{},
               svtkm::worklet::MaskIndices{ mask },
               refCells,
               cellNormals,
               visitedCells);
      }
    }
  }
};
}
} // end namespace svtkm::worklet


#endif // svtkm_m_worklet_OrientCellNormals_h
