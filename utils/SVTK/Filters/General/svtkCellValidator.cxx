/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellValidator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellValidator.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkPointData.h"
#include "svtkShortArray.h"

#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkPoints.h"

#include "svtkBezierCurve.h"
#include "svtkBezierHexahedron.h"
#include "svtkBezierQuadrilateral.h"
#include "svtkBezierTetra.h"
#include "svtkBezierTriangle.h"
#include "svtkBezierWedge.h"
#include "svtkBiQuadraticQuad.h"
#include "svtkBiQuadraticQuadraticHexahedron.h"
#include "svtkBiQuadraticQuadraticWedge.h"
#include "svtkBiQuadraticTriangle.h"
#include "svtkConvexPointSet.h"
#include "svtkCubicLine.h"
#include "svtkEmptyCell.h"
#include "svtkGenericCell.h"
#include "svtkHexagonalPrism.h"
#include "svtkHexahedron.h"
#include "svtkLagrangeCurve.h"
#include "svtkLagrangeHexahedron.h"
#include "svtkLagrangeQuadrilateral.h"
#include "svtkLagrangeTetra.h"
#include "svtkLagrangeTriangle.h"
#include "svtkLagrangeWedge.h"
#include "svtkLine.h"
#include "svtkPentagonalPrism.h"
#include "svtkPixel.h"
#include "svtkPolyLine.h"
#include "svtkPolyVertex.h"
#include "svtkPolygon.h"
#include "svtkPolyhedron.h"
#include "svtkPyramid.h"
#include "svtkQuad.h"
#include "svtkQuadraticEdge.h"
#include "svtkQuadraticHexahedron.h"
#include "svtkQuadraticLinearQuad.h"
#include "svtkQuadraticLinearWedge.h"
#include "svtkQuadraticPolygon.h"
#include "svtkQuadraticPyramid.h"
#include "svtkQuadraticQuad.h"
#include "svtkQuadraticTetra.h"
#include "svtkQuadraticTriangle.h"
#include "svtkQuadraticWedge.h"
#include "svtkTetra.h"
#include "svtkTriQuadraticHexahedron.h"
#include "svtkTriangle.h"
#include "svtkTriangleStrip.h"
#include "svtkVertex.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkUnstructuredGrid.h"

#include <cassert>
#include <cmath>
#include <map>
#include <sstream>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkCellValidator);

//----------------------------------------------------------------------------
svtkCellValidator::svtkCellValidator()
{
  this->Tolerance = FLT_EPSILON;
}

//----------------------------------------------------------------------------
namespace
{
bool PointsAreCoincident(double p[3], double q[3], double tolerance)
{
  return (std::abs(p[0] - q[0]) < tolerance && std::abs(p[1] - q[1]) < tolerance &&
    std::abs(p[2] - q[2]) < tolerance);
}

bool LineSegmentsIntersect(double p1[3], double p2[3], double q1[3], double q2[3], double tolerance)
{
  double u, v;
  static const int SVTK_YES_INTERSECTION = 2;
  if (svtkLine::Intersection3D(p1, p2, q1, q2, u, v) == SVTK_YES_INTERSECTION)
  {
    if ((std::abs(u) > tolerance && std::abs(u - 1.) > tolerance) ||
      (std::abs(v) > tolerance && std::abs(v - 1.) > tolerance))
    {
      return true;
    }
  }
  return false;
}
}

//----------------------------------------------------------------------------
bool svtkCellValidator::NoIntersectingEdges(svtkCell* cell, double tolerance)
{
  // Ensures no cell edges intersect.
  //
  // NB: To accommodate higher order cells, we need to first linearize the edges
  //     before testing their intersection.

  double p[2][3], x[2][3];
  svtkIdType nEdges = cell->GetNumberOfEdges();
  svtkCell* edge;
  svtkNew<svtkIdList> idList1, idList2;
  svtkNew<svtkPoints> points1, points2;
  int subId = -1;
  for (svtkIdType i = 0; i < nEdges; i++)
  {
    edge = cell->GetEdge(i);
    edge->Triangulate(subId, idList1.GetPointer(), points1.GetPointer());
    for (svtkIdType e1 = 0; e1 < points1->GetNumberOfPoints(); e1 += 2)
    {
      points1->GetPoint(e1, p[0]);
      points1->GetPoint(e1 + 1, p[1]);
      for (svtkIdType j = i + 1; j < nEdges; j++)
      {
        edge = cell->GetEdge(j);
        edge->Triangulate(subId, idList2.GetPointer(), points2.GetPointer());
        for (svtkIdType e2 = 0; e2 < points2->GetNumberOfPoints(); e2 += 2)
        {
          points2->GetPoint(e2, x[0]);
          points2->GetPoint(e2 + 1, x[1]);

          if (LineSegmentsIntersect(p[0], p[1], x[0], x[1], tolerance))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
namespace
{
bool TrianglesIntersect(double p1[3], double p2[3], double p3[3], double q1[3], double q2[3],
  double q3[3], double tolerance)
{
  if (svtkTriangle::TrianglesIntersect(p1, p2, p3, q1, q2, q3) == 1)
  {
    double* p[3] = { p1, p2, p3 };
    double* q[3] = { q1, q2, q3 };

    int nCoincidentPoints = 0;

    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        if (LineSegmentsIntersect(p[i], p[(i + 1) % 3], q[j], q[(j + 1) % 3], tolerance))
        {
          return false;
        }
        nCoincidentPoints += int(PointsAreCoincident(p[i], q[j], tolerance));
      }
    }
    return (nCoincidentPoints != 1 && nCoincidentPoints != 2);
  }
  return false;
}
}

//----------------------------------------------------------------------------
bool svtkCellValidator::NoIntersectingFaces(svtkCell* cell, double tolerance)
{
  // Ensures no cell faces intersect.

  double p[3][3], x[3][3];
  svtkIdType nFaces = cell->GetNumberOfFaces();
  svtkCell* face;
  svtkNew<svtkIdList> idList1, idList2;
  svtkNew<svtkPoints> points1, points2;
  int subId = -1;
  for (svtkIdType i = 0; i < nFaces; i++)
  {
    face = cell->GetFace(i);
    face->Triangulate(subId, idList1.GetPointer(), points1.GetPointer());
    for (svtkIdType e1 = 0; e1 < points1->GetNumberOfPoints(); e1 += 3)
    {
      points1->GetPoint(e1, p[0]);
      points1->GetPoint(e1 + 1, p[1]);
      points1->GetPoint(e1 + 2, p[2]);
      for (svtkIdType j = i + 1; j < nFaces; j++)
      {
        face = cell->GetFace(j);
        face->Triangulate(subId, idList2.GetPointer(), points2.GetPointer());
        for (svtkIdType e2 = 0; e2 < points2->GetNumberOfPoints(); e2 += 3)
        {
          points2->GetPoint(e2, x[0]);
          points2->GetPoint(e2 + 1, x[1]);
          points2->GetPoint(e2 + 2, x[2]);

          if (TrianglesIntersect(p[0], p[1], p[2], x[0], x[1], x[2], tolerance))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkCellValidator::ContiguousEdges(svtkCell* twoDimensionalCell, double tolerance)
{
  // Ensures that a two-dimensional cell's edges are contiguous.
  //
  // NB: we cannot simply test the values of point ids, since cells have the
  //     tricky habit of generating their edge cells on the fly and cell Ids are
  //     only congruent w.r.t. a single point array. To be thorough, we need to
  //     compare point values.

  assert(twoDimensionalCell->GetCellDimension() == 2);

  double points[4][3];
  double *p[2] = { points[0], points[1] }, *x[2] = { points[2], points[3] }, u, v;
  svtkCell* edge = twoDimensionalCell->GetEdge(0);
  svtkIdType nEdges = twoDimensionalCell->GetNumberOfEdges();
  // Need to use local indices, not global
  edge->GetPoints()->GetPoint(0, p[0]);
  edge->GetPoints()->GetPoint(1, p[1]);
  for (svtkIdType i = 0; i < nEdges; i++)
  {
    edge = twoDimensionalCell->GetEdge((i + 1) % nEdges);
    // Need to use local indices, not global
    edge->GetPoints()->GetPoint(0, x[0]);
    edge->GetPoints()->GetPoint(1, x[1]);

    static const int SVTK_NO_INTERSECTION = 0;
    if (svtkLine::Intersection3D(p[0], p[1], x[0], x[1], u, v) == SVTK_NO_INTERSECTION)
    {
      return false;
    }
    else if ((std::abs(u) > tolerance && std::abs(1. - u) > tolerance) ||
      (std::abs(v) > tolerance && std::abs(1. - v) > tolerance))
    {
      return false;
    }
    p[0] = x[0];
    p[1] = x[1];
  }
  return true;
}

//----------------------------------------------------------------------------
namespace
{
void Centroid(svtkCell* cell, double* centroid)
{
  // Return the centroid of a cell in world coordinates.
  static double weights[512];
  double pCenter[3];
  int subId = -1;
  cell->GetParametricCenter(pCenter);
  cell->EvaluateLocation(subId, pCenter, centroid, weights);
}

void Normal(svtkCell* twoDimensionalCell, double* normal)
{
  // Return the normal of a 2-dimensional cell.

  assert(twoDimensionalCell->GetCellDimension() == 2);

  svtkPolygon::ComputeNormal(twoDimensionalCell->GetPoints(), normal);
}
}

//----------------------------------------------------------------------------
bool svtkCellValidator::Convex(svtkCell* cell, double svtkNotUsed(tolerance))
{
  // Determine whether or not a cell is convex. svtkPolygon and svtkPolyhedron can
  // conform to any 2- and 3-dimensional cell, and both have IsConvex(). So, we
  // construct instances of these cells, populate them with the cell data, and
  // proceed with the convexity query.
  switch (cell->GetCellDimension())
  {
    case 0:
    case 1:
      return true;
    case 2:
      return svtkPolygon::IsConvex(cell->GetPoints());
    case 3:
    {
      if (svtkPolyhedron* polyhedron = svtkPolyhedron::SafeDownCast(cell))
      {
        return polyhedron->IsConvex();
      }
      svtkNew<svtkCellArray> polyhedronFaces;
      svtkIdType faces_n = cell->GetNumberOfFaces();
      for (svtkIdType i = 0; i < faces_n; i++)
      {
        polyhedronFaces->InsertNextCell(cell->GetFace(i));
      }
      svtkNew<svtkIdTypeArray> faceBuffer;
      polyhedronFaces->ExportLegacyFormat(faceBuffer);

      // Explanation of the mapping with an example of a cell containing 3 points:
      // The input is:
      // input_grid_ids      10      11     12
      // input_grid_points   (0.0)   (0.1)  (0.2)

      // cell_ids      11      12     10
      // cell_points   (0.1)  (0.2)   (0.0)

      // The output has to be:
      // new_grid_ids       0      1     2       ( grid ids cannot be set)
      // new_grid_points   (0.1)  (0.2)   (0.0)  ( set with cell>GetPoints())

      // polygon_cell_id    0      1      2
      // So the mapping for the new nodes is such that 11->0, 12->1 and 10->3
      // This mapping is used to update the node ids of the faces

      svtkIdType points_n = cell->GetNumberOfPoints();
      std::vector<svtkIdType> polyhedron_pointIds(points_n);
      std::unordered_map<int, int> node_mapping;
      for (svtkIdType i = 0; i < points_n; i++)
      {
        node_mapping.emplace(cell->PointIds->GetId(i), i);
        polyhedron_pointIds[i] = i;
      }

      // udpate the face ids
      svtkIdType cpt = 0;
      for (svtkIdType i = 0; i < faces_n; i++)
      {
        svtkIdType face_point_n = faceBuffer->GetPointer(0)[cpt];
        cpt++;
        for (svtkIdType j = 0; j < face_point_n; j++)
        {
          faceBuffer->GetPointer(0)[cpt] = node_mapping.at(faceBuffer->GetPointer(0)[cpt]);
          cpt++;
        }
      }

      svtkNew<svtkUnstructuredGrid> ugrid;
      ugrid->SetPoints(cell->GetPoints());
      ugrid->InsertNextCell(
        SVTK_POLYHEDRON, points_n, polyhedron_pointIds.data(), faces_n, faceBuffer->GetPointer(0));

      svtkPolyhedron* polyhedron = svtkPolyhedron::SafeDownCast(ugrid->GetCell(0));
      return polyhedron->IsConvex();
    }
    default:
      return false;
  }
}

//----------------------------------------------------------------------------
namespace
{
// The convention for three-dimensional cells is that the normal of each face
// cell is oriented outwards. Some cells break this convention and remain
// inconsistent to maintain backwards compatibility.
bool outwardOrientation(int cellType)
{
  if (cellType == SVTK_QUADRATIC_LINEAR_WEDGE || cellType == SVTK_BIQUADRATIC_QUADRATIC_WEDGE ||
    cellType == SVTK_QUADRATIC_WEDGE)
  {
    return false;
  }

  return true;
}
}

//----------------------------------------------------------------------------
bool svtkCellValidator::FacesAreOrientedCorrectly(svtkCell* threeDimensionalCell, double tolerance)
{
  // Ensure that a 3-dimensional cell's faces are oriented away from the
  // cell's centroid.

  assert(threeDimensionalCell->GetCellDimension() == 3);

  double faceNorm[3], norm[3], cellCentroid[3], faceCentroid[3];
  svtkCell* face;
  Centroid(threeDimensionalCell, cellCentroid);

  bool hasOutwardOrientation = outwardOrientation(threeDimensionalCell->GetCellType());

  for (svtkIdType i = 0; i < threeDimensionalCell->GetNumberOfFaces(); i++)
  {
    face = threeDimensionalCell->GetFace(i);
    // If the cell face is not valid, there's no point in continuing the test.
    if (svtkCellValidator::Check(face, tolerance) != State::Valid)
    {
      return false;
    }
    Normal(face, faceNorm);
    Centroid(face, faceCentroid);
    for (svtkIdType j = 0; j < 3; j++)
    {
      norm[j] = faceCentroid[j] - cellCentroid[j];
    }
    svtkMath::Normalize(norm);
    double dot = svtkMath::Dot(faceNorm, norm);

    if (hasOutwardOrientation == (dot < 0.))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkCell* cell, double tolerance)
{
  // Ensure the number of points is at least as great as the number of point ids
  if (cell->GetPoints()->GetNumberOfPoints() < cell->GetNumberOfPoints())
  {
    return State::WrongNumberOfPoints;
  }

  switch (cell->GetCellType())
  {

#define CheckCase(CellId, CellType)                                                                \
  case CellId:                                                                                     \
    return svtkCellValidator::Check(CellType::SafeDownCast(cell), tolerance)
    CheckCase(SVTK_EMPTY_CELL, svtkEmptyCell);
    CheckCase(SVTK_VERTEX, svtkVertex);
    CheckCase(SVTK_POLY_VERTEX, svtkPolyVertex);
    CheckCase(SVTK_LINE, svtkLine);
    CheckCase(SVTK_POLY_LINE, svtkPolyLine);
    CheckCase(SVTK_TRIANGLE, svtkTriangle);
    CheckCase(SVTK_TRIANGLE_STRIP, svtkTriangleStrip);
    CheckCase(SVTK_POLYGON, svtkPolygon);
    CheckCase(SVTK_PIXEL, svtkPixel);
    CheckCase(SVTK_QUAD, svtkQuad);
    CheckCase(SVTK_TETRA, svtkTetra);
    CheckCase(SVTK_VOXEL, svtkVoxel);
    CheckCase(SVTK_HEXAHEDRON, svtkHexahedron);
    CheckCase(SVTK_WEDGE, svtkWedge);
    CheckCase(SVTK_PYRAMID, svtkPyramid);
    CheckCase(SVTK_PENTAGONAL_PRISM, svtkPentagonalPrism);
    CheckCase(SVTK_HEXAGONAL_PRISM, svtkHexagonalPrism);
    CheckCase(SVTK_QUADRATIC_EDGE, svtkQuadraticEdge);
    CheckCase(SVTK_QUADRATIC_TRIANGLE, svtkQuadraticTriangle);
    CheckCase(SVTK_QUADRATIC_QUAD, svtkQuadraticQuad);
    CheckCase(SVTK_QUADRATIC_POLYGON, svtkQuadraticPolygon);
    CheckCase(SVTK_QUADRATIC_TETRA, svtkQuadraticTetra);
    CheckCase(SVTK_QUADRATIC_HEXAHEDRON, svtkQuadraticHexahedron);
    CheckCase(SVTK_QUADRATIC_WEDGE, svtkQuadraticWedge);
    CheckCase(SVTK_QUADRATIC_PYRAMID, svtkQuadraticPyramid);
    CheckCase(SVTK_BIQUADRATIC_QUAD, svtkBiQuadraticQuad);
    CheckCase(SVTK_TRIQUADRATIC_HEXAHEDRON, svtkTriQuadraticHexahedron);
    CheckCase(SVTK_QUADRATIC_LINEAR_QUAD, svtkQuadraticLinearQuad);
    CheckCase(SVTK_QUADRATIC_LINEAR_WEDGE, svtkQuadraticLinearWedge);
    CheckCase(SVTK_BIQUADRATIC_QUADRATIC_WEDGE, svtkBiQuadraticQuadraticWedge);
    CheckCase(SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, svtkBiQuadraticQuadraticHexahedron);
    CheckCase(SVTK_BIQUADRATIC_TRIANGLE, svtkBiQuadraticTriangle);
    CheckCase(SVTK_CUBIC_LINE, svtkCubicLine);
    CheckCase(SVTK_CONVEX_POINT_SET, svtkConvexPointSet);
    CheckCase(SVTK_POLYHEDRON, svtkPolyhedron);
    CheckCase(SVTK_LAGRANGE_CURVE, svtkLagrangeCurve);
    CheckCase(SVTK_LAGRANGE_TRIANGLE, svtkLagrangeTriangle);
    CheckCase(SVTK_LAGRANGE_QUADRILATERAL, svtkLagrangeQuadrilateral);
    CheckCase(SVTK_LAGRANGE_TETRAHEDRON, svtkLagrangeTetra);
    CheckCase(SVTK_LAGRANGE_HEXAHEDRON, svtkLagrangeHexahedron);
    CheckCase(SVTK_LAGRANGE_WEDGE, svtkLagrangeWedge);
    CheckCase(SVTK_BEZIER_CURVE, svtkBezierCurve);
    CheckCase(SVTK_BEZIER_TRIANGLE, svtkBezierTriangle);
    CheckCase(SVTK_BEZIER_QUADRILATERAL, svtkBezierQuadrilateral);
    CheckCase(SVTK_BEZIER_TETRAHEDRON, svtkBezierTetra);
    CheckCase(SVTK_BEZIER_HEXAHEDRON, svtkBezierHexahedron);
    CheckCase(SVTK_BEZIER_WEDGE, svtkBezierWedge);
#undef CheckCase

    default:
      return State::Valid;
  }
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkGenericCell* cell, double tolerance)
{
  return svtkCellValidator::Check(cell->GetRepresentativeCell(), tolerance);
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkEmptyCell*, double)
{
  return State::Valid;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkVertex* vertex, double svtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there is an underlying point id for the vertex
  if (vertex->GetNumberOfPoints() != 1)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkPolyVertex* polyVertex, double svtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there is an a single underlying point id for the polyVertex
  if (polyVertex->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLine* line, double svtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are two underlying point ids for the line
  if (line->GetNumberOfPoints() != 2)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkPolyLine* polyLine, double svtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are at least two underlying point ids for the polyLine
  if (polyLine->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are three underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkTriangleStrip* triangleStrip, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least three underlying point ids for the triangleStrip
  if (triangleStrip->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangleStrip, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkPolygon* polygon, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least three underlying point ids for the polygon
  if (polygon->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polygon, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(polygon, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  // Ensure that the polygon is convex
  if (!Convex(polygon, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkPixel* pixel, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the pixel
  if (pixel->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the voxel points are orthogonal and axis-aligned
  double p[4][3];
  for (svtkIdType i = 0; i < 4; i++)
  {
    pixel->GetPoints()->GetPoint(pixel->GetPointId(i), p[i]);
  }

  // pixel points are axis-aligned and orthogonal, so exactly one coordinate
  // must differ by a tolerance along its edges.
  static int edges[4][2] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 } };
  for (svtkIdType i = 0; i < 4; i++)
  {
    if ((std::abs(p[edges[i][0]][0] - p[edges[i][1]][0]) > tolerance) +
        (std::abs(p[edges[i][0]][1] - p[edges[i][1]][1]) > tolerance) +
        (std::abs(p[edges[i][0]][2] - p[edges[i][1]][2]) > tolerance) !=
      1)
    {
      state |= State::IntersectingEdges;
    }
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  // Ensure that the quad is convex
  if (!Convex(quad, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkTetra* tetra, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the tetra
  if (tetra->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetra, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetra, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkVoxel* voxel, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the voxel
  if (voxel->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the voxel points are orthogonal and axis-aligned
  double p[8][3];
  for (svtkIdType i = 0; i < 8; i++)
  {
    voxel->GetPoints()->GetPoint(voxel->GetPointId(i), p[i]);
  }

  // voxel points are axis-aligned and orthogonal, so exactly one coordinate
  // must differ by a tolerance along its edges.
  static int edges[12][2] = {
    { 0, 1 },
    { 1, 3 },
    { 2, 3 },
    { 0, 2 },
    { 4, 5 },
    { 5, 7 },
    { 6, 7 },
    { 4, 6 },
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 },
  };
  for (svtkIdType i = 0; i < 12; i++)
  {
    if ((std::abs(p[edges[i][0]][0] - p[edges[i][1]][0]) > tolerance) +
        (std::abs(p[edges[i][0]][1] - p[edges[i][1]][1]) > tolerance) +
        (std::abs(p[edges[i][0]][2] - p[edges[i][1]][2]) > tolerance) !=
      1)
    {
      state |= State::IntersectingEdges;
    }
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eight underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the hex is convex
  if (!Convex(hex, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the wedge is convex
  if (!Convex(wedge, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkPyramid* pyramid, double tolerance)
{
  State state = State::Valid;

  // Ensure there are five underlying point ids for the pyramid
  if (pyramid->GetNumberOfPoints() != 5)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pyramid, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the pyramid is convex
  if (!Convex(pyramid, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pyramid, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkPentagonalPrism* pentagonalPrism, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the pentagonal prism
  if (pentagonalPrism->GetNumberOfPoints() != 10)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pentagonalPrism, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pentagonalPrism, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the pentagonal prism is convex
  if (!Convex(pentagonalPrism, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the prism's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pentagonalPrism, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkHexagonalPrism* hexagonalPrism, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the hexagonal prism
  if (hexagonalPrism->GetNumberOfPoints() != 12)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexagonalPrism, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexagonalPrism, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the hexagonal prism is convex
  if (!Convex(hexagonalPrism, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the prism's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexagonalPrism, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticEdge* edge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are three underlying point ids for the edge
  if (edge->GetNumberOfPoints() != 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(edge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eight underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticPolygon* polygon, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least six underlying point ids for the polygon
  if (polygon->GetNumberOfPoints() < 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polygon, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(polygon, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticTetra* tetra, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the tetra
  if (tetra->GetNumberOfPoints() != 10)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetra, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetra, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetra's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetra, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 20)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are fifteen underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 15)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticPyramid* pyramid, double tolerance)
{
  State state = State::Valid;

  // Ensure there are thirteen underlying point ids for the pyramid
  if (pyramid->GetNumberOfPoints() != 13)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pyramid, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBiQuadraticQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are nine underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 9)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkTriQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty-seven underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 27)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticLinearQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkQuadraticLinearWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twelve underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 12)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkBiQuadraticQuadraticWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eighteen underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 18)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkBiQuadraticQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty-four underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 24)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBiQuadraticTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are seven underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 7)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkCubicLine* line, double svtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the edge
  if (line->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkConvexPointSet* pointSet, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the point set
  if (pointSet->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the point set is convex
  if (!Convex(pointSet, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkPolyhedron* polyhedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the polyhedron
  if (polyhedron->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polyhedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(polyhedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the polyhedron is convex
  if (!Convex(polyhedron, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the polyhedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(polyhedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLagrangeCurve* curve, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the curve
  if (curve->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(curve, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLagrangeTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(triangle, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkLagrangeQuadrilateral* quadrilateral, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the quadrilateral
  if (quadrilateral->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quadrilateral, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(quadrilateral, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLagrangeTetra* tetrahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the tetrahedron
  if (tetrahedron->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetrahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetrahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetrahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetrahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLagrangeHexahedron* hexahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the hexahedron
  if (hexahedron->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkLagrangeWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBezierCurve* curve, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the curve
  if (curve->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(curve, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBezierTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(triangle, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(
  svtkBezierQuadrilateral* quadrilateral, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the quadrilateral
  if (quadrilateral->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quadrilateral, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(quadrilateral, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBezierTetra* tetrahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the tetrahedron
  if (tetrahedron->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetrahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetrahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetrahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetrahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBezierHexahedron* hexahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the hexahedron
  if (hexahedron->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
svtkCellValidator::State svtkCellValidator::Check(svtkBezierWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
int svtkCellValidator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // copy the input to the output as a starting point
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  svtkNew<svtkShortArray> stateArray;
  stateArray->SetNumberOfComponents(1);
  stateArray->SetName("ValidityState"); // set the name of the value
  stateArray->SetNumberOfTuples(input->GetNumberOfCells());

  svtkGenericCell* cell = svtkGenericCell::New();
  svtkCellIterator* it = input->NewCellIterator();
  svtkIdType counter = 0;
  State state;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell);
    state = Check(cell, this->Tolerance);
    stateArray->SetValue(counter, static_cast<short>(state));
    if (state != State::Valid)
    {
      std::stringstream s;
      cell->Print(s);
      this->PrintState(state, s, svtkIndent(0));
      svtkOutputWindowDisplayText(s.str().c_str());
    }
    ++counter;
  }
  cell->Delete();
  it->Delete();

  output->GetCellData()->AddArray(stateArray.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
void svtkCellValidator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkCellValidator::PrintState(svtkCellValidator::State state, ostream& os, svtkIndent indent)
{
  if (state == State::Valid)
  {
    os << indent << "Cell is valid.\n";
  }
  else
  {
    os << indent << "Cell is invalid for the following reason(s):\n";

    if ((state & svtkCellValidator::State::WrongNumberOfPoints) ==
      svtkCellValidator::State::WrongNumberOfPoints)
    {
      os << indent << "  - Wrong number of points\n";
    }
    if ((state & svtkCellValidator::State::IntersectingEdges) ==
      svtkCellValidator::State::IntersectingEdges)
    {
      os << indent << "  - Intersecting edges\n";
    }
    if ((state & svtkCellValidator::State::NoncontiguousEdges) ==
      svtkCellValidator::State::NoncontiguousEdges)
    {
      os << indent << "  - Noncontiguous edges\n";
    }
    if ((state & svtkCellValidator::State::Nonconvex) == svtkCellValidator::State::Nonconvex)
    {
      os << indent << "  - Nonconvex\n";
    }
    if ((state & svtkCellValidator::State::FacesAreOrientedIncorrectly) ==
      svtkCellValidator::State::FacesAreOrientedIncorrectly)
    {
      os << indent << "  - Faces are oriented incorrectly\n";
    }
  }
}
