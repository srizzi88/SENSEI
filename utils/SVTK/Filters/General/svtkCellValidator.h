/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellValidator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellValidator
 * @brief   validates cells in a dataset
 *
 *
 * svtkCellValidator accepts as input a dataset and adds integral cell data
 * to it corresponding to the "validity" of each cell. The validity field
 * encodes a bitfield for identifying problems that prevent a cell from standard
 * use, including:
 *
 *   WrongNumberOfPoints: filters assume that a cell has access to the
 *                        appropriate number of points that comprise it. This
 *                        assumption is often tacit, resulting in unexpected
 *                        behavior when the condition is not met. This check
 *                        simply confirms that the cell has the minimum number
 *                        of points needed to describe it.
 *
 *   IntersectingEdges: cells that incorrectly describe the order of their
 *                      points often manifest with intersecting edges or
 *                      intersecting faces. Given a tolerance, this check
 *                      ensures that two edges from a two-dimensional cell
 *                      are separated by at least the tolerance (discounting
 *                      end-to-end connections).
 *
 *   IntersectingFaces: cells that incorrectly describe the order of their
 *                      points often manifest with intersecting edges or
 *                      intersecting faces. Given a tolerance, this check
 *                      ensures that two faces from a three-dimensional cell
 *                      do not intersect.
 *
 *   NoncontiguousEdges: another symptom of incorrect point ordering within a
 *                       cell is the presence of noncontiguous edges where
 *                       contiguous edges are otherwise expected. Given a
 *                       tolerance, this check ensures that edges around the
 *                       perimeter of a two-dimensional cell are contiguous.
 *
 *   Nonconvex: many algorithms implicitly require that all input three-
 *              dimensional cells be convex. This check uses the generic
 *              convexity checkers implemented in svtkPolygon and svtkPolyhedron
 *              to test this requirement.
 *
 *   FacesAreOrientedIncorrectly: All three-dimensional cells have an implicit
 *                                expectation for the orientation of their
 *                                faces. While the convention is unfortunately
 *                                inconsistent across cell types, it is usually
 *                                required that cell faces point outward. This
 *                                check tests that the faces of a cell point in
 *                                the direction required by the cell type,
 *                                taking into account the cell types with
 *                                nonstandard orientation requirements.
 *
 *
 * @sa
 * svtkCellQuality
 */

#ifndef svtkCellValidator_h
#define svtkCellValidator_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkCell;
class svtkGenericCell;
class svtkEmptyCell;
class svtkVertex;
class svtkPolyVertex;
class svtkLine;
class svtkPolyLine;
class svtkTriangle;
class svtkTriangleStrip;
class svtkPolygon;
class svtkPixel;
class svtkQuad;
class svtkTetra;
class svtkVoxel;
class svtkHexahedron;
class svtkWedge;
class svtkPyramid;
class svtkPentagonalPrism;
class svtkHexagonalPrism;
class svtkQuadraticEdge;
class svtkQuadraticTriangle;
class svtkQuadraticQuad;
class svtkQuadraticPolygon;
class svtkQuadraticTetra;
class svtkQuadraticHexahedron;
class svtkQuadraticWedge;
class svtkQuadraticPyramid;
class svtkBiQuadraticQuad;
class svtkTriQuadraticHexahedron;
class svtkQuadraticLinearQuad;
class svtkQuadraticLinearWedge;
class svtkBiQuadraticQuadraticWedge;
class svtkBiQuadraticQuadraticHexahedron;
class svtkBiQuadraticTriangle;
class svtkCubicLine;
class svtkConvexPointSet;
class svtkPolyhedron;
class svtkLagrangeCurve;
class svtkLagrangeTriangle;
class svtkLagrangeQuadrilateral;
class svtkLagrangeTetra;
class svtkLagrangeHexahedron;
class svtkLagrangeWedge;
class svtkBezierCurve;
class svtkBezierTriangle;
class svtkBezierQuadrilateral;
class svtkBezierTetra;
class svtkBezierHexahedron;
class svtkBezierWedge;

class SVTKFILTERSGENERAL_EXPORT svtkCellValidator : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkCellValidator, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Description:
  // Construct to compute the validity of cells.
  static svtkCellValidator* New();

  enum State : short
  {
    Valid = 0x0,
    WrongNumberOfPoints = 0x01,
    IntersectingEdges = 0x02,
    IntersectingFaces = 0x04,
    NoncontiguousEdges = 0x08,
    Nonconvex = 0x10,
    FacesAreOrientedIncorrectly = 0x20,
  };

  friend inline State operator&(State a, State b)
  {
    return static_cast<State>(static_cast<short>(a) & static_cast<short>(b));
  }
  friend inline State operator|(State a, State b)
  {
    return static_cast<State>(static_cast<short>(a) | static_cast<short>(b));
  }
  friend inline State& operator&=(State& a, State b) { return a = a & b; }

  friend inline State& operator|=(State& a, State b) { return a = a | b; }

  static void PrintState(State state, ostream& os, svtkIndent indent);

  static State Check(svtkGenericCell*, double tolerance);
  static State Check(svtkCell*, double tolerance);

  static State Check(svtkEmptyCell*, double tolerance);
  static State Check(svtkVertex*, double tolerance);
  static State Check(svtkPolyVertex*, double tolerance);
  static State Check(svtkLine*, double tolerance);
  static State Check(svtkPolyLine*, double tolerance);
  static State Check(svtkTriangle*, double tolerance);
  static State Check(svtkTriangleStrip*, double tolerance);
  static State Check(svtkPolygon*, double tolerance);
  static State Check(svtkPixel*, double tolerance);
  static State Check(svtkQuad*, double tolerance);
  static State Check(svtkTetra*, double tolerance);
  static State Check(svtkVoxel*, double tolerance);
  static State Check(svtkHexahedron*, double tolerance);
  static State Check(svtkWedge*, double tolerance);
  static State Check(svtkPyramid*, double tolerance);
  static State Check(svtkPentagonalPrism*, double tolerance);
  static State Check(svtkHexagonalPrism*, double tolerance);
  static State Check(svtkQuadraticEdge*, double tolerance);
  static State Check(svtkQuadraticTriangle*, double tolerance);
  static State Check(svtkQuadraticQuad*, double tolerance);
  static State Check(svtkQuadraticPolygon*, double tolerance);
  static State Check(svtkQuadraticTetra*, double tolerance);
  static State Check(svtkQuadraticHexahedron*, double tolerance);
  static State Check(svtkQuadraticWedge*, double tolerance);
  static State Check(svtkQuadraticPyramid*, double tolerance);
  static State Check(svtkBiQuadraticQuad*, double tolerance);
  static State Check(svtkTriQuadraticHexahedron*, double tolerance);
  static State Check(svtkQuadraticLinearQuad*, double tolerance);
  static State Check(svtkQuadraticLinearWedge*, double tolerance);
  static State Check(svtkBiQuadraticQuadraticWedge*, double tolerance);
  static State Check(svtkBiQuadraticQuadraticHexahedron*, double tolerance);
  static State Check(svtkBiQuadraticTriangle*, double tolerance);
  static State Check(svtkCubicLine*, double tolerance);
  static State Check(svtkConvexPointSet*, double tolerance);
  static State Check(svtkPolyhedron*, double tolerance);
  static State Check(svtkLagrangeCurve*, double tolerance);
  static State Check(svtkLagrangeTriangle*, double tolerance);
  static State Check(svtkLagrangeQuadrilateral*, double tolerance);
  static State Check(svtkLagrangeTetra*, double tolerance);
  static State Check(svtkLagrangeHexahedron*, double tolerance);
  static State Check(svtkLagrangeWedge*, double tolerance);
  static State Check(svtkBezierCurve*, double tolerance);
  static State Check(svtkBezierTriangle*, double tolerance);
  static State Check(svtkBezierQuadrilateral*, double tolerance);
  static State Check(svtkBezierTetra*, double tolerance);
  static State Check(svtkBezierHexahedron*, double tolerance);
  static State Check(svtkBezierWedge*, double tolerance);

  //@{
  /**
   * Set/Get the tolerance. This value is used as an epsilon for floating point
   * equality checks throughout the cell checking process. The default value is
   * FLT_EPSILON.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Tolerance, double);
  //@}

protected:
  svtkCellValidator();
  ~svtkCellValidator() override {}

  double Tolerance;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  static bool NoIntersectingEdges(svtkCell* cell, double tolerance);
  static bool NoIntersectingFaces(svtkCell* cell, double tolerance);
  static bool ContiguousEdges(svtkCell* twoDimensionalCell, double tolerance);
  static bool Convex(svtkCell* cell, double tolerance);
  static bool FacesAreOrientedCorrectly(svtkCell* threeDimensionalCell, double tolerance);

private:
  svtkCellValidator(const svtkCellValidator&) = delete;
  void operator=(const svtkCellValidator&) = delete;
};

#endif
