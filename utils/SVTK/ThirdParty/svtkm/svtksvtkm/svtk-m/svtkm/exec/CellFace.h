//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_CellFace_h
#define svtk_m_exec_CellFace_h

#include <svtkm/Assert.h>
#include <svtkm/CellShape.h>
#include <svtkm/Types.h>
#include <svtkm/exec/FunctorBase.h>
#include <svtkm/internal/Assume.h>

namespace svtkm
{
namespace exec
{

namespace detail
{

class CellFaceTables
{
public:
  static constexpr svtkm::Int32 MAX_FACE_SIZE = 4;
  static constexpr svtkm::Int32 MAX_NUM_FACES = 6;

  SVTKM_EXEC svtkm::Int32 NumFaces(svtkm::Int32 cellShapeId) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::Int32 numFaces[svtkm::NUMBER_OF_CELL_SHAPES] = {
      // NumFaces
      0, //  0: CELL_SHAPE_EMPTY
      0, //  1: CELL_SHAPE_VERTEX
      0, //  2: Unused
      0, //  3: CELL_SHAPE_LINE
      0, //  4: CELL_SHAPE_POLY_LINE
      0, //  5: CELL_SHAPE_TRIANGLE
      0, //  6: Unused
      0, //  7: CELL_SHAPE_POLYGON
      0, //  8: Unused
      0, //  9: CELL_SHAPE_QUAD
      4, // 10: CELL_SHAPE_TETRA
      0, // 11: Unused
      6, // 12: CELL_SHAPE_HEXAHEDRON
      5, // 13: CELL_SHAPE_WEDGE
      5  // 14: CELL_SHAPE_PYRAMID
    };
    return numFaces[cellShapeId];
  }

  SVTKM_EXEC svtkm::Int32 NumPointsInFace(svtkm::Int32 cellShapeId, svtkm::Int32 faceIndex) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::Int32
      numPointsInFace[svtkm::NUMBER_OF_CELL_SHAPES][MAX_NUM_FACES] = {
        // NumPointsInFace
        { -1, -1, -1, -1, -1, -1 }, //  0: CELL_SHAPE_EMPTY
        { -1, -1, -1, -1, -1, -1 }, //  1: CELL_SHAPE_VERTEX
        { -1, -1, -1, -1, -1, -1 }, //  2: Unused
        { -1, -1, -1, -1, -1, -1 }, //  3: CELL_SHAPE_LINE
        { -1, -1, -1, -1, -1, -1 }, //  4: CELL_SHAPE_POLY_LINE
        { -1, -1, -1, -1, -1, -1 }, //  5: CELL_SHAPE_TRIANGLE
        { -1, -1, -1, -1, -1, -1 }, //  6: Unused
        { -1, -1, -1, -1, -1, -1 }, //  7: CELL_SHAPE_POLYGON
        { -1, -1, -1, -1, -1, -1 }, //  8: Unused
        { -1, -1, -1, -1, -1, -1 }, //  9: CELL_SHAPE_QUAD
        { 3, 3, 3, 3, -1, -1 },     // 10: CELL_SHAPE_TETRA
        { -1, -1, -1, -1, -1, -1 }, // 11: Unused
        { 4, 4, 4, 4, 4, 4 },       // 12: CELL_SHAPE_HEXAHEDRON
        { 3, 3, 4, 4, 4, -1 },      // 13: CELL_SHAPE_WEDGE
        { 4, 3, 3, 3, 3, -1 }       // 14: CELL_SHAPE_PYRAMID
      };
    return numPointsInFace[cellShapeId][faceIndex];
  }

  SVTKM_EXEC svtkm::Int32 PointsInFace(svtkm::Int32 cellShapeId,
                                     svtkm::Int32 faceIndex,
                                     svtkm::Int32 localPointIndex) const
  {
    // clang-format off
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::Int32 pointsInFace[svtkm::NUMBER_OF_CELL_SHAPES][MAX_NUM_FACES]
                                                  [MAX_FACE_SIZE] =
    {
      // PointsInFace
      //  0: CELL_SHAPE_EMPTY
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  1: CELL_SHAPE_VERTEX
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  2: Unused
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  3: CELL_SHAPE_LINE
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  4: CELL_SHAPE_POLY_LINE
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  5: CELL_SHAPE_TRIANGLE
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  6: Unused
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  7: CELL_SHAPE_POLYGON
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  8: Unused
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      //  9: CELL_SHAPE_QUAD
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      // 10: CELL_SHAPE_TETRA
      { { 0, 1, 3, -1 }, { 1, 2, 3, -1 }, { 2, 0, 3, -1 },
        { 0, 2, 1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      // 11: Unused
      { { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 },
        { -1, -1, -1, -1 }, { -1, -1, -1, -1 }, { -1, -1, -1, -1 } },
      // 12: CELL_SHAPE_HEXAHEDRON
      { { 0, 4, 7, 3 }, { 1, 2, 6, 5 }, { 0, 1, 5, 4 },
        { 3, 7, 6, 2 }, { 0, 3, 2, 1 }, { 4, 5, 6, 7 } },
      // 13: CELL_SHAPE_WEDGE
      { { 0, 1, 2, -1 }, { 3, 5, 4, -1 }, { 0, 3, 4, 1 },
        { 1, 4, 5, 2 },  { 2, 5, 3, 0 }, { -1, -1, -1, -1 } },
      // 14: CELL_SHAPE_PYRAMID
      { { 0, 3, 2, 1 }, { 0, 1, 4, -1 }, { 1, 2, 4, -1 },
        { 2, 3, 4, -1 }, { 3, 0, 4, -1 },{ -1, -1, -1, -1 } }
                                                          // clang-format on
                                                        };
    return pointsInFace[cellShapeId][faceIndex][localPointIndex];
  }
};

} // namespace detail

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::IdComponent CellFaceNumberOfFaces(CellShapeTag shape,
                                                                const svtkm::exec::FunctorBase&)
{
  (void)shape; //C4100 false positive workaround
  detail::CellFaceTables table;
  return table.NumFaces(shape.Id);
}

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::IdComponent CellFaceNumberOfPoints(
  svtkm::IdComponent faceIndex,
  CellShapeTag shape,
  const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSUME(faceIndex >= 0);
  SVTKM_ASSUME(faceIndex < detail::CellFaceTables::MAX_NUM_FACES);
  if (faceIndex >= svtkm::exec::CellFaceNumberOfFaces(shape, worklet))
  {
    worklet.RaiseError("Invalid face number.");
    return 0;
  }
  detail::CellFaceTables table;
  return table.NumPointsInFace(shape.Id, faceIndex);
}

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::UInt8 CellFaceShape(svtkm::IdComponent faceIndex,
                                                  CellShapeTag shape,
                                                  const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSUME(faceIndex >= 0);
  SVTKM_ASSUME(faceIndex < detail::CellFaceTables::MAX_NUM_FACES);
  switch (CellFaceNumberOfPoints(faceIndex, shape, worklet))
  {
    case 3:
      return svtkm::CELL_SHAPE_TRIANGLE;
    case 4:
      return svtkm::CELL_SHAPE_QUAD;
    default:
      return svtkm::CELL_SHAPE_POLYGON;
  }
}

template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::IdComponent CellFaceLocalIndex(svtkm::IdComponent pointIndex,
                                                             svtkm::IdComponent faceIndex,
                                                             CellShapeTag shape,
                                                             const svtkm::exec::FunctorBase& worklet)
{
  svtkm::IdComponent numPointsInFace = svtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, worklet);
  if (numPointsInFace < 1)
  {
    // An invalid face. We should already have gotten an error from
    // CellFaceNumberOfPoints.
    return -1;
  }

  detail::CellFaceTables table;
  return table.PointsInFace(shape.Id, faceIndex, pointIndex);
}

/// \brief Returns a canonical identifier for a cell face
///
/// Given information about a cell face and the global point indices for that cell, returns a
/// svtkm::Id3 that contains values that are unique to that face. The values for two faces will be
/// the same if and only if the faces contain the same points.
///
/// Note that this property is only true if the mesh is conforming. That is, any two neighboring
/// cells that share a face have the same points on that face. This preculdes 2 faces sharing more
/// than a single point or single edge.
///
template <typename CellShapeTag, typename GlobalPointIndicesVecType>
static inline SVTKM_EXEC svtkm::Id3 CellFaceCanonicalId(
  svtkm::IdComponent faceIndex,
  CellShapeTag shape,
  const GlobalPointIndicesVecType& globalPointIndicesVec,
  const svtkm::exec::FunctorBase& worklet)
{
  const svtkm::IdComponent numPointsInFace =
    svtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, worklet);
  if (numPointsInFace == 0)
  {
    // An invalid face. We should already have gotten an error from
    // CellFaceNumberOfPoints.
    return svtkm::Id3(0);
  }

  detail::CellFaceTables table;
  //Sort the first 3 face points/nodes in ascending order
  svtkm::Id3 sorted(globalPointIndicesVec[table.PointsInFace(shape.Id, faceIndex, 0)],
                   globalPointIndicesVec[table.PointsInFace(shape.Id, faceIndex, 1)],
                   globalPointIndicesVec[table.PointsInFace(shape.Id, faceIndex, 2)]);
  svtkm::Id temp;
  if (sorted[0] > sorted[2])
  {
    temp = sorted[0];
    sorted[0] = sorted[2];
    sorted[2] = temp;
  }
  if (sorted[0] > sorted[1])
  {
    temp = sorted[0];
    sorted[0] = sorted[1];
    sorted[1] = temp;
  }
  if (sorted[1] > sorted[2])
  {
    temp = sorted[1];
    sorted[1] = sorted[2];
    sorted[2] = temp;
  }

  // Check the rest of the points to see if they are in the lowest 3
  for (svtkm::IdComponent pointIndex = 3; pointIndex < numPointsInFace; pointIndex++)
  {
    svtkm::Id nextPoint = globalPointIndicesVec[table.PointsInFace(shape.Id, faceIndex, pointIndex)];
    if (nextPoint < sorted[2])
    {
      if (nextPoint < sorted[1])
      {
        sorted[2] = sorted[1];
        if (nextPoint < sorted[0])
        {
          sorted[1] = sorted[0];
          sorted[0] = nextPoint;
        }
        else // nextPoint > P0, nextPoint < P1
        {
          sorted[1] = nextPoint;
        }
      }
      else // nextPoint > P1, nextPoint < P2
      {
        sorted[2] = nextPoint;
      }
    }
    else // nextPoint > P2
    {
      // Do nothing. nextPoint not in top 3.
    }
  }

  return sorted;
}
}
} // namespace svtkm::exec

#endif //svtk_m_exec_CellFace_h
