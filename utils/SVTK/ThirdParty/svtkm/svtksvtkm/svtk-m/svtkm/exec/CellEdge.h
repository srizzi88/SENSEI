//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_CellEdge_h
#define svtk_m_exec_CellEdge_h

#include <svtkm/Assert.h>
#include <svtkm/CellShape.h>
#include <svtkm/CellTraits.h>
#include <svtkm/Types.h>
#include <svtkm/exec/FunctorBase.h>
#include <svtkm/internal/Assume.h>

namespace svtkm
{
namespace exec
{

namespace detail
{

class CellEdgeTables
{
public:
  static constexpr svtkm::Int32 MAX_NUM_EDGES = 12;

public:
  SVTKM_EXEC svtkm::Int32 NumEdges(svtkm::Int32 cellShapeId) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::Int32 numEdges[svtkm::NUMBER_OF_CELL_SHAPES] = {
      // NumEdges
      0,  //  0: CELL_SHAPE_EMPTY
      0,  //  1: CELL_SHAPE_VERTEX
      0,  //  2: Unused
      0,  //  3: CELL_SHAPE_LINE
      0,  //  4: CELL_SHAPE_POLY_LINE
      3,  //  5: CELL_SHAPE_TRIANGLE
      0,  //  6: Unused
      -1, //  7: CELL_SHAPE_POLYGON  ---special case---
      0,  //  8: Unused
      4,  //  9: CELL_SHAPE_QUAD
      6,  // 10: CELL_SHAPE_TETRA
      0,  // 11: Unused
      12, // 12: CELL_SHAPE_HEXAHEDRON
      9,  // 13: CELL_SHAPE_WEDGE
      8   // 14: CELL_SHAPE_PYRAMID
    };
    return numEdges[cellShapeId];
  }

  SVTKM_EXEC svtkm::Int32 PointsInEdge(svtkm::Int32 cellShapeId,
                                     svtkm::Int32 edgeIndex,
                                     svtkm::Int32 localPointIndex) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::Int32
      pointsInEdge[svtkm::NUMBER_OF_CELL_SHAPES][MAX_NUM_EDGES][2] = {
        // clang-format off
        //  0: CELL_SHAPE_EMPTY
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  1: CELL_SHAPE_VERTEX
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  2: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  3: CELL_SHAPE_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  4: CELL_SHAPE_POLY_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  5: CELL_SHAPE_TRIANGLE
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  6: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  7: CELL_SHAPE_POLYGON  --- special case ---
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  8: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  9: CELL_SHAPE_QUAD
        { { 0, 1 },   { 1, 2 },   { 2, 3 },   { 3, 0 },   { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 10: CELL_SHAPE_TETRA
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { 0, 3 },   { 1, 3 },   { 2, 3 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 11: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 12: CELL_SHAPE_HEXAHEDRON
        { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 },
          { 7, 6 }, { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } },
        // 13: CELL_SHAPE_WEDGE
        { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 4 },   { 4, 5 },   { 5, 3 },
          { 0, 3 }, { 1, 4 }, { 2, 5 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 14: CELL_SHAPE_PYRAMID
        { { 0, 1 }, { 1, 2 }, { 2, 3 },   { 3, 0 },   { 0, 4 },   { 1, 4 },
          { 2, 4 }, { 3, 4 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }
        // clang-format on
      };

    return pointsInEdge[cellShapeId][edgeIndex][localPointIndex];
  }
};

} // namespace detail

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::IdComponent CellEdgeNumberOfEdges(svtkm::IdComponent numPoints,
                                                                CellShapeTag,
                                                                const svtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSERT(numPoints == svtkm::CellTraits<CellShapeTag>::NUM_POINTS);
  return detail::CellEdgeTables{}.NumEdges(CellShapeTag::Id);
}

static inline SVTKM_EXEC svtkm::IdComponent CellEdgeNumberOfEdges(svtkm::IdComponent numPoints,
                                                                svtkm::CellShapeTagPolygon,
                                                                const svtkm::exec::FunctorBase&)
{
  SVTKM_ASSUME(numPoints > 0);
  return numPoints;
}

static inline SVTKM_EXEC svtkm::IdComponent CellEdgeNumberOfEdges(svtkm::IdComponent numPoints,
                                                                svtkm::CellShapeTagPolyLine,
                                                                const svtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSUME(numPoints > 0);
  return detail::CellEdgeTables{}.NumEdges(svtkm::CELL_SHAPE_POLY_LINE);
}

static inline SVTKM_EXEC svtkm::IdComponent CellEdgeNumberOfEdges(
  svtkm::IdComponent numPoints,
  svtkm::CellShapeTagGeneric shape,
  const svtkm::exec::FunctorBase& worklet)
{
  if (shape.Id == svtkm::CELL_SHAPE_POLYGON)
  {
    return CellEdgeNumberOfEdges(numPoints, svtkm::CellShapeTagPolygon(), worklet);
  }
  else if (shape.Id == svtkm::CELL_SHAPE_POLY_LINE)
  {
    return CellEdgeNumberOfEdges(numPoints, svtkm::CellShapeTagPolyLine(), worklet);
  }
  else
  {
    return detail::CellEdgeTables{}.NumEdges(shape.Id);
  }
}

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::IdComponent CellEdgeLocalIndex(svtkm::IdComponent numPoints,
                                                             svtkm::IdComponent pointIndex,
                                                             svtkm::IdComponent edgeIndex,
                                                             CellShapeTag shape,
                                                             const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSUME(pointIndex >= 0);
  SVTKM_ASSUME(pointIndex < 2);
  SVTKM_ASSUME(edgeIndex >= 0);
  SVTKM_ASSUME(edgeIndex < detail::CellEdgeTables::MAX_NUM_EDGES);
  if (edgeIndex >= svtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, worklet))
  {
    worklet.RaiseError("Invalid edge number.");
    return 0;
  }

  detail::CellEdgeTables table;
  return table.PointsInEdge(CellShapeTag::Id, edgeIndex, pointIndex);
}

static inline SVTKM_EXEC svtkm::IdComponent CellEdgeLocalIndex(svtkm::IdComponent numPoints,
                                                             svtkm::IdComponent pointIndex,
                                                             svtkm::IdComponent edgeIndex,
                                                             svtkm::CellShapeTagPolygon,
                                                             const svtkm::exec::FunctorBase&)
{
  SVTKM_ASSUME(numPoints >= 3);
  SVTKM_ASSUME(pointIndex >= 0);
  SVTKM_ASSUME(pointIndex < 2);
  SVTKM_ASSUME(edgeIndex >= 0);
  SVTKM_ASSUME(edgeIndex < numPoints);

  if (edgeIndex + pointIndex < numPoints)
  {
    return edgeIndex + pointIndex;
  }
  else
  {
    return 0;
  }
}

static inline SVTKM_EXEC svtkm::IdComponent CellEdgeLocalIndex(svtkm::IdComponent numPoints,
                                                             svtkm::IdComponent pointIndex,
                                                             svtkm::IdComponent edgeIndex,
                                                             svtkm::CellShapeTagGeneric shape,
                                                             const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSUME(pointIndex >= 0);
  SVTKM_ASSUME(pointIndex < 2);
  SVTKM_ASSUME(edgeIndex >= 0);
  SVTKM_ASSUME(edgeIndex < detail::CellEdgeTables::MAX_NUM_EDGES);

  if (shape.Id == svtkm::CELL_SHAPE_POLYGON)
  {
    return CellEdgeLocalIndex(
      numPoints, pointIndex, edgeIndex, svtkm::CellShapeTagPolygon(), worklet);
  }
  else
  {
    detail::CellEdgeTables table;
    if (edgeIndex >= table.NumEdges(shape.Id))
    {
      worklet.RaiseError("Invalid edge number.");
      return 0;
    }

    return table.PointsInEdge(shape.Id, edgeIndex, pointIndex);
  }
}

/// \brief Returns a canonical identifier for a cell edge
///
/// Given information about a cell edge and the global point indices for that cell, returns a
/// svtkm::Id2 that contains values that are unique to that edge. The values for two edges will be
/// the same if and only if the edges contain the same points.
///
template <typename CellShapeTag, typename GlobalPointIndicesVecType>
static inline SVTKM_EXEC svtkm::Id2 CellEdgeCanonicalId(
  svtkm::IdComponent numPoints,
  svtkm::IdComponent edgeIndex,
  CellShapeTag shape,
  const GlobalPointIndicesVecType& globalPointIndicesVec,
  const svtkm::exec::FunctorBase& worklet)
{
  svtkm::Id pointIndex0 =
    globalPointIndicesVec[svtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, worklet)];
  svtkm::Id pointIndex1 =
    globalPointIndicesVec[svtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, worklet)];
  if (pointIndex0 < pointIndex1)
  {
    return svtkm::Id2(pointIndex0, pointIndex1);
  }
  else
  {
    return svtkm::Id2(pointIndex1, pointIndex0);
  }
}
}
} // namespace svtkm::exec

#endif //svtk_m_exec_CellFaces_h
