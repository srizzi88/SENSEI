/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellValidator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkCellValidator

#include <svtkCellValidator.h>

#include "svtkEmptyCell.h"
#include "svtkHexagonalPrism.h"
#include "svtkHexahedron.h"
#include "svtkLine.h"
#include "svtkPentagonalPrism.h"
#include "svtkPixel.h"
#include "svtkPolyLine.h"
#include "svtkPolyVertex.h"
#include "svtkPolygon.h"
#include "svtkPolyhedron.h"
#include "svtkPyramid.h"
#include "svtkQuad.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include "svtkTriangleStrip.h"
#include "svtkVertex.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"

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

#include "svtkBiQuadraticQuad.h"
#include "svtkBiQuadraticQuadraticHexahedron.h"
#include "svtkBiQuadraticQuadraticWedge.h"
#include "svtkBiQuadraticTriangle.h"
#include "svtkTriQuadraticHexahedron.h"

#include "svtkCubicLine.h"

#include "svtkLagrangeCurve.h"
#include "svtkLagrangeHexahedron.h"
#include "svtkLagrangeQuadrilateral.h"
#include "svtkLagrangeTetra.h"
#include "svtkLagrangeTriangle.h"
#include "svtkLagrangeWedge.h"

#include "svtkBezierCurve.h"
#include "svtkBezierHexahedron.h"
#include "svtkBezierQuadrilateral.h"
#include "svtkBezierTetra.h"
#include "svtkBezierTriangle.h"
#include "svtkBezierWedge.h"

#include "svtkCellArray.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkPoints.h"
#include "svtkUnstructuredGrid.h"

#include <svtkActor.h>
#include <svtkDataSetMapper.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

static svtkSmartPointer<svtkEmptyCell> MakeEmptyCell();
static svtkSmartPointer<svtkVertex> MakeVertex();
static svtkSmartPointer<svtkPolyVertex> MakePolyVertex();
static svtkSmartPointer<svtkLine> MakeLine();
static svtkSmartPointer<svtkPolyLine> MakePolyLine();
static svtkSmartPointer<svtkTriangle> MakeTriangle();
static svtkSmartPointer<svtkTriangleStrip> MakeTriangleStrip();
static svtkSmartPointer<svtkPolygon> MakePolygon();
static svtkSmartPointer<svtkQuad> MakeQuad();
static svtkSmartPointer<svtkPixel> MakePixel();
static svtkSmartPointer<svtkVoxel> MakeVoxel();
static svtkSmartPointer<svtkHexahedron> MakeHexahedron();
static svtkSmartPointer<svtkHexahedron> MakeHexahedronConvexityNonTrivial();
static svtkSmartPointer<svtkHexahedron> MakeBrokenHexahedron();
static svtkSmartPointer<svtkPyramid> MakePyramid();
static svtkSmartPointer<svtkTetra> MakeTetra();
static svtkSmartPointer<svtkWedge> MakeWedge();
static svtkSmartPointer<svtkPentagonalPrism> MakePentagonalPrism();
static svtkSmartPointer<svtkHexagonalPrism> MakeHexagonalPrism();
static svtkSmartPointer<svtkPolyhedron> MakeCube();
static svtkSmartPointer<svtkPolyhedron> MakeDodecahedron();

static svtkSmartPointer<svtkQuadraticEdge> MakeQuadraticEdge();
static svtkSmartPointer<svtkQuadraticHexahedron> MakeQuadraticHexahedron();
static svtkSmartPointer<svtkQuadraticPolygon> MakeQuadraticPolygon();
static svtkSmartPointer<svtkQuadraticLinearQuad> MakeQuadraticLinearQuad();
static svtkSmartPointer<svtkQuadraticLinearWedge> MakeQuadraticLinearWedge();
static svtkSmartPointer<svtkQuadraticPyramid> MakeQuadraticPyramid();
static svtkSmartPointer<svtkQuadraticQuad> MakeQuadraticQuad();
static svtkSmartPointer<svtkQuadraticTetra> MakeQuadraticTetra();
static svtkSmartPointer<svtkQuadraticTriangle> MakeQuadraticTriangle();
static svtkSmartPointer<svtkQuadraticWedge> MakeQuadraticWedge();

static svtkSmartPointer<svtkBiQuadraticQuad> MakeBiQuadraticQuad();
static svtkSmartPointer<svtkBiQuadraticQuadraticHexahedron> MakeBiQuadraticQuadraticHexahedron();
static svtkSmartPointer<svtkBiQuadraticQuadraticWedge> MakeBiQuadraticQuadraticWedge();
static svtkSmartPointer<svtkBiQuadraticTriangle> MakeBiQuadraticTriangle();
static svtkSmartPointer<svtkTriQuadraticHexahedron> MakeTriQuadraticHexahedron();
static svtkSmartPointer<svtkCubicLine> MakeCubicLine();

static svtkSmartPointer<svtkLagrangeCurve> MakeLagrangeCurve();
static svtkSmartPointer<svtkLagrangeTriangle> MakeLagrangeTriangle();
static svtkSmartPointer<svtkLagrangeTriangle> MakeBrokenLagrangeTriangle();
static svtkSmartPointer<svtkLagrangeQuadrilateral> MakeLagrangeQuadrilateral();
static svtkSmartPointer<svtkLagrangeTetra> MakeLagrangeTetra();
static svtkSmartPointer<svtkLagrangeHexahedron> MakeLagrangeHexahedron();
static svtkSmartPointer<svtkLagrangeWedge> MakeLagrangeWedge();

static svtkSmartPointer<svtkBezierCurve> MakeBezierCurve();
static svtkSmartPointer<svtkBezierTriangle> MakeBezierTriangle();
static svtkSmartPointer<svtkBezierQuadrilateral> MakeBezierQuadrilateral();
static svtkSmartPointer<svtkBezierTetra> MakeBezierTetra();
static svtkSmartPointer<svtkBezierHexahedron> MakeBezierHexahedron();
static svtkSmartPointer<svtkBezierWedge> MakeBezierWedge();
//----------------------------------------------------------------------------

int TestCellValidator(int, char*[])
{
  svtkSmartPointer<svtkEmptyCell> emptyCell = MakeEmptyCell();
  svtkSmartPointer<svtkVertex> vertex = MakeVertex();
  svtkSmartPointer<svtkPolyVertex> polyVertex = MakePolyVertex();
  svtkSmartPointer<svtkLine> line = MakeLine();
  svtkSmartPointer<svtkPolyLine> polyLine = MakePolyLine();
  svtkSmartPointer<svtkTriangle> triangle = MakeTriangle();
  svtkSmartPointer<svtkTriangleStrip> triangleStrip = MakeTriangleStrip();
  svtkSmartPointer<svtkPolygon> polygon = MakePolygon();
  svtkSmartPointer<svtkQuad> quad = MakeQuad();
  svtkSmartPointer<svtkPixel> pixel = MakePixel();
  svtkSmartPointer<svtkVoxel> voxel = MakeVoxel();
  svtkSmartPointer<svtkHexahedron> hexahedron = MakeHexahedron();
  svtkSmartPointer<svtkHexahedron> hexahedronConvexityNonTrivial =
    MakeHexahedronConvexityNonTrivial();
  svtkSmartPointer<svtkPyramid> pyramid = MakePyramid();
  svtkSmartPointer<svtkTetra> tetra = MakeTetra();
  svtkSmartPointer<svtkWedge> wedge = MakeWedge();
  svtkSmartPointer<svtkPentagonalPrism> pentagonalPrism = MakePentagonalPrism();
  svtkSmartPointer<svtkHexagonalPrism> hexagonalPrism = MakeHexagonalPrism();
  svtkSmartPointer<svtkPolyhedron> poly1 = MakeCube();
  svtkSmartPointer<svtkPolyhedron> poly2 = MakeDodecahedron();

  svtkSmartPointer<svtkQuadraticEdge> quadraticEdge = MakeQuadraticEdge();
  svtkSmartPointer<svtkQuadraticHexahedron> quadraticHexahedron = MakeQuadraticHexahedron();
  svtkSmartPointer<svtkQuadraticPolygon> quadraticPolygon = MakeQuadraticPolygon();
  svtkSmartPointer<svtkQuadraticLinearQuad> quadraticLinearQuad = MakeQuadraticLinearQuad();
  svtkSmartPointer<svtkQuadraticLinearWedge> quadraticLinearWedge = MakeQuadraticLinearWedge();
  svtkSmartPointer<svtkQuadraticPyramid> quadraticPyramid = MakeQuadraticPyramid();
  svtkSmartPointer<svtkQuadraticQuad> quadraticQuad = MakeQuadraticQuad();
  svtkSmartPointer<svtkQuadraticTetra> quadraticTetra = MakeQuadraticTetra();
  svtkSmartPointer<svtkQuadraticTriangle> quadraticTriangle = MakeQuadraticTriangle();
  svtkSmartPointer<svtkQuadraticWedge> quadraticWedge = MakeQuadraticWedge();

  svtkSmartPointer<svtkBiQuadraticQuad> biQuadraticQuad = MakeBiQuadraticQuad();
  svtkSmartPointer<svtkBiQuadraticQuadraticHexahedron> biQuadraticQuadraticHexahedron =
    MakeBiQuadraticQuadraticHexahedron();
  svtkSmartPointer<svtkBiQuadraticQuadraticWedge> biQuadraticQuadraticWedge =
    MakeBiQuadraticQuadraticWedge();
  svtkSmartPointer<svtkBiQuadraticTriangle> biQuadraticTriangle = MakeBiQuadraticTriangle();
  svtkSmartPointer<svtkTriQuadraticHexahedron> triQuadraticHexahedron = MakeTriQuadraticHexahedron();
  svtkSmartPointer<svtkCubicLine> cubicLine = MakeCubicLine();

  svtkSmartPointer<svtkLagrangeCurve> lagrangeCurve = MakeLagrangeCurve();
  svtkSmartPointer<svtkLagrangeTriangle> lagrangeTriangle = MakeLagrangeTriangle();
  svtkSmartPointer<svtkLagrangeTriangle> brokenLagrangeTriangle = MakeBrokenLagrangeTriangle();
  svtkSmartPointer<svtkLagrangeQuadrilateral> lagrangeQuadrilateral = MakeLagrangeQuadrilateral();
  svtkSmartPointer<svtkLagrangeTetra> lagrangeTetra = MakeLagrangeTetra();
  svtkSmartPointer<svtkLagrangeHexahedron> lagrangeHexahedron = MakeLagrangeHexahedron();
  svtkSmartPointer<svtkLagrangeWedge> lagrangeWedge = MakeLagrangeWedge();

  svtkSmartPointer<svtkBezierCurve> bezierCurve = MakeBezierCurve();
  svtkSmartPointer<svtkBezierTriangle> bezierTriangle = MakeBezierTriangle();
  svtkSmartPointer<svtkBezierQuadrilateral> bezierQuadrilateral = MakeBezierQuadrilateral();
  svtkSmartPointer<svtkBezierTetra> bezierTetra = MakeBezierTetra();
  svtkSmartPointer<svtkBezierHexahedron> bezierHexahedron = MakeBezierHexahedron();
  svtkSmartPointer<svtkBezierWedge> bezierWedge = MakeBezierWedge();

  svtkCellValidator::State state;

#define CheckCell(cellPtr)                                                                         \
  state = svtkCellValidator::Check(cellPtr, FLT_EPSILON);                                           \
  if (state != svtkCellValidator::State::Valid)                                                     \
  {                                                                                                \
    cellPtr->Print(std::cout);                                                                     \
    svtkCellValidator::PrintState(state, std::cout, svtkIndent(0));                                  \
    return EXIT_FAILURE;                                                                           \
  }

  CheckCell(emptyCell);
  CheckCell(vertex);
  CheckCell(polyVertex);
  CheckCell(line);
  CheckCell(polyLine);
  CheckCell(triangle);
  CheckCell(triangleStrip);
  CheckCell(polygon);
  CheckCell(pixel);
  CheckCell(quad);
  CheckCell(tetra);
  CheckCell(voxel);
  CheckCell(hexahedron);
  CheckCell(hexahedronConvexityNonTrivial);
  CheckCell(wedge);
  CheckCell(pyramid);
  CheckCell(pentagonalPrism);
  CheckCell(hexagonalPrism);
  CheckCell(poly1);
  CheckCell(poly2);
  CheckCell(quadraticEdge);
  CheckCell(quadraticHexahedron);
  CheckCell(quadraticPolygon);
  CheckCell(quadraticLinearQuad);
  CheckCell(quadraticLinearWedge);
  CheckCell(quadraticPyramid);
  CheckCell(quadraticQuad);
  CheckCell(quadraticTetra);
  CheckCell(quadraticTriangle);
  CheckCell(quadraticWedge);
  CheckCell(quadraticWedge);
  CheckCell(biQuadraticQuad);
  CheckCell(biQuadraticQuadraticHexahedron);
  CheckCell(biQuadraticQuadraticWedge);
  CheckCell(biQuadraticTriangle);
  CheckCell(cubicLine);
  CheckCell(triQuadraticHexahedron);
  CheckCell(lagrangeCurve);
  CheckCell(lagrangeTriangle);
  CheckCell(lagrangeQuadrilateral);
  CheckCell(lagrangeTetra);
  CheckCell(lagrangeHexahedron);
  CheckCell(lagrangeWedge);
  CheckCell(bezierCurve);
  CheckCell(bezierTriangle);
  CheckCell(bezierQuadrilateral);
  CheckCell(bezierTetra);
  CheckCell(bezierHexahedron);
  CheckCell(bezierWedge);
#undef CheckCell

  state = svtkCellValidator::Check(MakeBrokenHexahedron(), FLT_EPSILON);
  if ((state & svtkCellValidator::State::IntersectingEdges) !=
    svtkCellValidator::State::IntersectingEdges)
  {
    svtkCellValidator::PrintState(state, std::cout, svtkIndent(0));
    return EXIT_FAILURE;
  }

  state = svtkCellValidator::Check(MakeBrokenLagrangeTriangle(), FLT_EPSILON);
  if ((state & svtkCellValidator::State::IntersectingEdges) !=
    svtkCellValidator::State::IntersectingEdges)
  {
    svtkCellValidator::PrintState(state, std::cout, svtkIndent(0));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

svtkSmartPointer<svtkEmptyCell> MakeEmptyCell()
{
  svtkSmartPointer<svtkEmptyCell> anEmptyCell = svtkSmartPointer<svtkEmptyCell>::New();
  return anEmptyCell;
}

svtkSmartPointer<svtkVertex> MakeVertex()
{
  svtkSmartPointer<svtkVertex> aVertex = svtkSmartPointer<svtkVertex>::New();
  aVertex->GetPointIds()->SetId(0, 0);
  aVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);

  return aVertex;
  ;
}

svtkSmartPointer<svtkPolyVertex> MakePolyVertex()
{
  svtkSmartPointer<svtkPolyVertex> aPolyVertex = svtkSmartPointer<svtkPolyVertex>::New();
  aPolyVertex->GetPointIds()->SetNumberOfIds(2);
  aPolyVertex->GetPointIds()->SetId(0, 0);
  aPolyVertex->GetPointIds()->SetId(1, 1);

  aPolyVertex->GetPoints()->SetNumberOfPoints(2);
  aPolyVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyVertex->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);

  return aPolyVertex;
  ;
}

svtkSmartPointer<svtkLine> MakeLine()
{
  svtkSmartPointer<svtkLine> aLine = svtkSmartPointer<svtkLine>::New();
  aLine->GetPointIds()->SetId(0, 0);
  aLine->GetPointIds()->SetId(1, 1);
  aLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aLine->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);
  return aLine;
  ;
}

svtkSmartPointer<svtkPolyLine> MakePolyLine()
{
  svtkSmartPointer<svtkPolyLine> aPolyLine = svtkSmartPointer<svtkPolyLine>::New();
  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0, 0);
  aPolyLine->GetPointIds()->SetId(1, 1);
  aPolyLine->GetPointIds()->SetId(2, 2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(2, 10.0, 30.0, 40.0);

  return aPolyLine;
  ;
}

svtkSmartPointer<svtkTriangle> MakeTriangle()
{
  svtkSmartPointer<svtkTriangle> aTriangle = svtkSmartPointer<svtkTriangle>::New();
  aTriangle->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aTriangle->GetPointIds()->SetId(0, 0);
  aTriangle->GetPointIds()->SetId(1, 1);
  aTriangle->GetPointIds()->SetId(2, 2);
  return aTriangle;
}

svtkSmartPointer<svtkTriangleStrip> MakeTriangleStrip()
{
  svtkSmartPointer<svtkTriangleStrip> aTriangleStrip = svtkSmartPointer<svtkTriangleStrip>::New();
  aTriangleStrip->GetPointIds()->SetNumberOfIds(4);
  aTriangleStrip->GetPointIds()->SetId(0, 0);
  aTriangleStrip->GetPointIds()->SetId(1, 1);
  aTriangleStrip->GetPointIds()->SetId(2, 2);
  aTriangleStrip->GetPointIds()->SetId(3, 3);

  aTriangleStrip->GetPoints()->SetNumberOfPoints(4);
  aTriangleStrip->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(3, 13.0, 10.0, 10.0);

  return aTriangleStrip;
}

svtkSmartPointer<svtkPolygon> MakePolygon()
{
  svtkSmartPointer<svtkPolygon> aPolygon = svtkSmartPointer<svtkPolygon>::New();
  aPolygon->GetPointIds()->SetNumberOfIds(4);
  aPolygon->GetPointIds()->SetId(0, 0);
  aPolygon->GetPointIds()->SetId(1, 1);
  aPolygon->GetPointIds()->SetId(2, 2);
  aPolygon->GetPointIds()->SetId(3, 3);

  aPolygon->GetPoints()->SetNumberOfPoints(4);
  aPolygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(1, 10.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aPolygon->GetPoints()->SetPoint(3, 0.0, 10.0, 0.0);

  return aPolygon;
}

svtkSmartPointer<svtkQuad> MakeQuad()
{
  svtkSmartPointer<svtkQuad> aQuad = svtkSmartPointer<svtkQuad>::New();
  aQuad->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aQuad->GetPoints()->SetPoint(3, -10.0, 10.0, 0.0);
  aQuad->GetPointIds()->SetId(0, 0);
  aQuad->GetPointIds()->SetId(1, 1);
  aQuad->GetPointIds()->SetId(2, 2);
  aQuad->GetPointIds()->SetId(2, 3);
  return aQuad;
}

svtkSmartPointer<svtkPixel> MakePixel()
{
  svtkSmartPointer<svtkPixel> aPixel = svtkSmartPointer<svtkPixel>::New();
  aPixel->GetPointIds()->SetId(0, 0);
  aPixel->GetPointIds()->SetId(1, 1);
  aPixel->GetPointIds()->SetId(2, 2);
  aPixel->GetPointIds()->SetId(3, 3);

  aPixel->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(2, 10.0, 12.0, 10.0);
  aPixel->GetPoints()->SetPoint(3, 12.0, 12.0, 10.0);
  return aPixel;
}

svtkSmartPointer<svtkVoxel> MakeVoxel()
{
  svtkSmartPointer<svtkVoxel> aVoxel = svtkSmartPointer<svtkVoxel>::New();
  aVoxel->GetPointIds()->SetId(0, 0);
  aVoxel->GetPointIds()->SetId(1, 1);
  aVoxel->GetPointIds()->SetId(2, 2);
  aVoxel->GetPointIds()->SetId(3, 3);
  aVoxel->GetPointIds()->SetId(4, 4);
  aVoxel->GetPointIds()->SetId(5, 5);
  aVoxel->GetPointIds()->SetId(6, 6);
  aVoxel->GetPointIds()->SetId(7, 7);

  aVoxel->GetPoints()->SetPoint(0, 10, 10, 10);
  aVoxel->GetPoints()->SetPoint(1, 12, 10, 10);
  aVoxel->GetPoints()->SetPoint(2, 10, 12, 10);
  aVoxel->GetPoints()->SetPoint(3, 12, 12, 10);
  aVoxel->GetPoints()->SetPoint(4, 10, 10, 12);
  aVoxel->GetPoints()->SetPoint(5, 12, 10, 12);
  aVoxel->GetPoints()->SetPoint(6, 10, 12, 12);
  aVoxel->GetPoints()->SetPoint(7, 12, 12, 12);
  return aVoxel;
}

svtkSmartPointer<svtkHexahedron> MakeHexahedron()
{
  svtkSmartPointer<svtkHexahedron> aHexahedron = svtkSmartPointer<svtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 2);
  aHexahedron->GetPointIds()->SetId(3, 3);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

  aHexahedron->GetPoints()->SetPoint(0, 10, 10, 10);
  aHexahedron->GetPoints()->SetPoint(1, 12, 10, 10);
  aHexahedron->GetPoints()->SetPoint(2, 12, 12, 10);
  aHexahedron->GetPoints()->SetPoint(3, 10, 12, 10);
  aHexahedron->GetPoints()->SetPoint(4, 10, 10, 12);
  aHexahedron->GetPoints()->SetPoint(5, 12, 10, 12);
  aHexahedron->GetPoints()->SetPoint(6, 12, 12, 12);
  aHexahedron->GetPoints()->SetPoint(7, 10, 12, 12);

  return aHexahedron;
}

svtkSmartPointer<svtkHexahedron> MakeHexahedronConvexityNonTrivial()
{
  // Example that was failing before, if now fixed and tested
  // https://gitlab.kitware.com/svtk/svtk/-/issues/17673
  svtkSmartPointer<svtkHexahedron> aHexahedron = svtkSmartPointer<svtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 2);
  aHexahedron->GetPointIds()->SetId(3, 3);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

  aHexahedron->GetPoints()->SetPoint(0, -2.9417226413, -0.92284313965, 4.5809917214);
  aHexahedron->GetPoints()->SetPoint(1, -3.0207607208, -0.84291999288, 4.357055109);
  aHexahedron->GetPoints()->SetPoint(2, -3.1077984177, -0.31259201362, 4.8124331347);
  aHexahedron->GetPoints()->SetPoint(3, -2.9320660211, -0.86238701507, 4.7197960612);
  aHexahedron->GetPoints()->SetPoint(4, -2.8375199741, -0.57697632408, 3.8069219868);
  aHexahedron->GetPoints()->SetPoint(5, -3.1669520923, -0.64026224489, 3.8129245089);
  aHexahedron->GetPoints()->SetPoint(6, -3.1935454463, -0.017891697066, 4.8277744194);
  aHexahedron->GetPoints()->SetPoint(7, -2.8265109805, -0.51675730395, 3.9006508868);

  return aHexahedron;
}

svtkSmartPointer<svtkHexahedron> MakeBrokenHexahedron()
{
  svtkSmartPointer<svtkHexahedron> aHexahedron = svtkSmartPointer<svtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 3);
  aHexahedron->GetPointIds()->SetId(3, 2);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

  aHexahedron->GetPoints()->SetPoint(1, 10, 10, 10);
  aHexahedron->GetPoints()->SetPoint(0, 12, 10, 10);
  aHexahedron->GetPoints()->SetPoint(2, 12, 12, 10);
  aHexahedron->GetPoints()->SetPoint(3, 10, 12, 10);
  aHexahedron->GetPoints()->SetPoint(4, 10, 10, 12);
  aHexahedron->GetPoints()->SetPoint(5, 12, 10, 12);
  aHexahedron->GetPoints()->SetPoint(6, 12, 12, 12);
  aHexahedron->GetPoints()->SetPoint(7, 10, 12, 12);

  return aHexahedron;
}

svtkSmartPointer<svtkPyramid> MakePyramid()
{
  svtkSmartPointer<svtkPyramid> aPyramid = svtkSmartPointer<svtkPyramid>::New();
  aPyramid->GetPointIds()->SetId(0, 0);
  aPyramid->GetPointIds()->SetId(1, 1);
  aPyramid->GetPointIds()->SetId(2, 2);
  aPyramid->GetPointIds()->SetId(3, 3);
  aPyramid->GetPointIds()->SetId(4, 4);

  aPyramid->GetPoints()->SetPoint(0, 0, 0, 0);
  aPyramid->GetPoints()->SetPoint(1, 1, 0, 0);
  aPyramid->GetPoints()->SetPoint(2, 1, 1, 0);
  aPyramid->GetPoints()->SetPoint(3, 0, 1, 0);
  aPyramid->GetPoints()->SetPoint(4, .5, .5, 1);

  return aPyramid;
}

svtkSmartPointer<svtkQuadraticPyramid> MakeQuadraticPyramid()
{
  svtkSmartPointer<svtkQuadraticPyramid> aPyramid = svtkSmartPointer<svtkQuadraticPyramid>::New();
  for (int i = 0; i < 13; ++i)
  {
    aPyramid->GetPointIds()->SetId(i, i);
  }

  aPyramid->GetPoints()->SetPoint(0, 0, 0, 0);
  aPyramid->GetPoints()->SetPoint(1, 1, 0, 0);
  aPyramid->GetPoints()->SetPoint(2, 1, 1, 0);
  aPyramid->GetPoints()->SetPoint(3, 0, 1, 0);
  aPyramid->GetPoints()->SetPoint(4, .5, .5, 1);

  aPyramid->GetPoints()->SetPoint(5, 0.5, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(6, 1.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(7, 0.5, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(8, 0.0, 0.5, 0.0);

  aPyramid->GetPoints()->SetPoint(9, 0.5, 0.5, 0.5);
  aPyramid->GetPoints()->SetPoint(10, 0.75, 0.5, 0.5);
  aPyramid->GetPoints()->SetPoint(11, 0.75, 0.75, 0.5);
  aPyramid->GetPoints()->SetPoint(12, 0.5, 0.75, 0.5);

  return aPyramid;
}

svtkSmartPointer<svtkQuadraticEdge> MakeQuadraticEdge()
{
  svtkSmartPointer<svtkQuadraticEdge> anEdge = svtkSmartPointer<svtkQuadraticEdge>::New();
  for (int i = 0; i < 3; ++i)
  {
    anEdge->GetPointIds()->SetId(i, i);
  }

  anEdge->GetPoints()->SetPoint(0, 0, 0, 0);
  anEdge->GetPoints()->SetPoint(1, 1, 0, 0);
  anEdge->GetPoints()->SetPoint(2, .5, 0, 0);

  return anEdge;
}

svtkSmartPointer<svtkQuadraticHexahedron> MakeQuadraticHexahedron()
{
  svtkSmartPointer<svtkQuadraticHexahedron> aHexahedron =
    svtkSmartPointer<svtkQuadraticHexahedron>::New();
  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + svtkMath::Random(-.1, .1));
  }
  return aHexahedron;
}

svtkSmartPointer<svtkBiQuadraticQuadraticHexahedron> MakeBiQuadraticQuadraticHexahedron()
{
  svtkSmartPointer<svtkBiQuadraticQuadraticHexahedron> aHexahedron =
    svtkSmartPointer<svtkBiQuadraticQuadraticHexahedron>::New();
  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + svtkMath::Random(-.1, .1));
  }
  return aHexahedron;
}

svtkSmartPointer<svtkTriQuadraticHexahedron> MakeTriQuadraticHexahedron()
{
  svtkSmartPointer<svtkTriQuadraticHexahedron> aHexahedron =
    svtkSmartPointer<svtkTriQuadraticHexahedron>::New();
  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + svtkMath::Random(-.1, .1));
  }
  return aHexahedron;
}

svtkSmartPointer<svtkQuadraticPolygon> MakeQuadraticPolygon()
{
  svtkSmartPointer<svtkQuadraticPolygon> aPolygon = svtkSmartPointer<svtkQuadraticPolygon>::New();

  aPolygon->GetPointIds()->SetNumberOfIds(8);
  aPolygon->GetPointIds()->SetId(0, 0);
  aPolygon->GetPointIds()->SetId(1, 1);
  aPolygon->GetPointIds()->SetId(2, 2);
  aPolygon->GetPointIds()->SetId(3, 3);
  aPolygon->GetPointIds()->SetId(4, 4);
  aPolygon->GetPointIds()->SetId(5, 5);
  aPolygon->GetPointIds()->SetId(6, 6);
  aPolygon->GetPointIds()->SetId(7, 7);

  aPolygon->GetPoints()->SetNumberOfPoints(8);
  aPolygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(1, 2.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(2, 2.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(4, 1.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(5, 2.0, 1.0, 0.0);
  aPolygon->GetPoints()->SetPoint(6, 1.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(7, 0.0, 1.0, 0.0);
  aPolygon->GetPoints()->SetPoint(5, 3.0, 1.0, 0.0);
  return aPolygon;
}

svtkSmartPointer<svtkQuadraticLinearQuad> MakeQuadraticLinearQuad()
{
  svtkSmartPointer<svtkQuadraticLinearQuad> aLinearQuad =
    svtkSmartPointer<svtkQuadraticLinearQuad>::New();
  double* pcoords = aLinearQuad->GetParametricCoords();
  for (int i = 0; i < aLinearQuad->GetNumberOfPoints(); ++i)
  {
    aLinearQuad->GetPointIds()->SetId(i, i);
    aLinearQuad->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aLinearQuad;
}

svtkSmartPointer<svtkQuadraticLinearWedge> MakeQuadraticLinearWedge()
{
  svtkSmartPointer<svtkQuadraticLinearWedge> aLinearWedge =
    svtkSmartPointer<svtkQuadraticLinearWedge>::New();
  double* pcoords = aLinearWedge->GetParametricCoords();
  for (int i = 0; i < 12; ++i)
  {
    aLinearWedge->GetPointIds()->SetId(i, i);
    aLinearWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aLinearWedge;
}

svtkSmartPointer<svtkQuadraticQuad> MakeQuadraticQuad()
{
  svtkSmartPointer<svtkQuadraticQuad> aQuad = svtkSmartPointer<svtkQuadraticQuad>::New();
  double* pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < 8; ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1), *(pcoords + 3 * i + 2));
  }
  return aQuad;
}

svtkSmartPointer<svtkQuadraticTetra> MakeQuadraticTetra()
{
  svtkSmartPointer<svtkQuadraticTetra> aTetra = svtkSmartPointer<svtkQuadraticTetra>::New();
  double* pcoords = aTetra->GetParametricCoords();
  for (int i = 0; i < 10; ++i)
  {
    aTetra->GetPointIds()->SetId(i, i);
    aTetra->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + svtkMath::Random(-.1, .1));
  }
  return aTetra;
}

svtkSmartPointer<svtkQuadraticTriangle> MakeQuadraticTriangle()
{
  svtkSmartPointer<svtkQuadraticTriangle> aTriangle = svtkSmartPointer<svtkQuadraticTriangle>::New();
  double* pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aTriangle;
}

svtkSmartPointer<svtkBiQuadraticTriangle> MakeBiQuadraticTriangle()
{
  svtkSmartPointer<svtkBiQuadraticTriangle> aTriangle =
    svtkSmartPointer<svtkBiQuadraticTriangle>::New();
  double* pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aTriangle;
}

svtkSmartPointer<svtkBiQuadraticQuad> MakeBiQuadraticQuad()
{
  svtkSmartPointer<svtkBiQuadraticQuad> aQuad = svtkSmartPointer<svtkBiQuadraticQuad>::New();
  double* pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < aQuad->GetNumberOfPoints(); ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + svtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + svtkMath::Random(-.1, .1), *(pcoords + 3 * i + 2));
  }
  return aQuad;
}

svtkSmartPointer<svtkCubicLine> MakeCubicLine()
{
  svtkSmartPointer<svtkCubicLine> aLine = svtkSmartPointer<svtkCubicLine>::New();
  double* pcoords = aLine->GetParametricCoords();
  for (int i = 0; i < aLine->GetNumberOfPoints(); ++i)
  {
    aLine->GetPointIds()->SetId(i, i);
    aLine->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aLine;
}

svtkSmartPointer<svtkQuadraticWedge> MakeQuadraticWedge()
{
  svtkSmartPointer<svtkQuadraticWedge> aWedge = svtkSmartPointer<svtkQuadraticWedge>::New();
  double* pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aWedge;
}

svtkSmartPointer<svtkBiQuadraticQuadraticWedge> MakeBiQuadraticQuadraticWedge()
{
  svtkSmartPointer<svtkBiQuadraticQuadraticWedge> aWedge =
    svtkSmartPointer<svtkBiQuadraticQuadraticWedge>::New();
  double* pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }
  return aWedge;
}

svtkSmartPointer<svtkTetra> MakeTetra()
{
  svtkSmartPointer<svtkTetra> aTetra = svtkSmartPointer<svtkTetra>::New();
  aTetra->GetPointIds()->SetId(0, 0);
  aTetra->GetPointIds()->SetId(1, 1);
  aTetra->GetPointIds()->SetId(2, 2);
  aTetra->GetPointIds()->SetId(3, 3);
  aTetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);
  return aTetra;
}

svtkSmartPointer<svtkWedge> MakeWedge()
{
  svtkSmartPointer<svtkWedge> aWedge = svtkSmartPointer<svtkWedge>::New();
  aWedge->GetPointIds()->SetId(0, 0);
  aWedge->GetPointIds()->SetId(1, 1);
  aWedge->GetPointIds()->SetId(2, 2);
  aWedge->GetPointIds()->SetId(3, 3);
  aWedge->GetPointIds()->SetId(4, 4);
  aWedge->GetPointIds()->SetId(5, 5);

  aWedge->GetPoints()->SetPoint(0, 0, 1, 0);
  aWedge->GetPoints()->SetPoint(1, 0, 0, 0);
  aWedge->GetPoints()->SetPoint(2, 0, .5, .5);
  aWedge->GetPoints()->SetPoint(3, 1, 1, 0);
  aWedge->GetPoints()->SetPoint(4, 1, 0.0, 0.0);
  aWedge->GetPoints()->SetPoint(5, 1, .5, .5);

  return aWedge;
}

svtkSmartPointer<svtkPolyhedron> MakeCube()
{
  svtkSmartPointer<svtkPolyhedron> aCube = svtkSmartPointer<svtkPolyhedron>::New();

  // create polyhedron (cube)
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  aCube->GetPointIds()->SetNumberOfIds(8);
  aCube->GetPointIds()->SetId(0, 0);
  aCube->GetPointIds()->SetId(1, 1);
  aCube->GetPointIds()->SetId(2, 2);
  aCube->GetPointIds()->SetId(3, 3);
  aCube->GetPointIds()->SetId(4, 4);
  aCube->GetPointIds()->SetId(5, 5);
  aCube->GetPointIds()->SetId(6, 6);
  aCube->GetPointIds()->SetId(7, 7);

  aCube->GetPoints()->SetNumberOfPoints(8);
  aCube->GetPoints()->SetPoint(0, -1.0, -1.0, -1.0);
  aCube->GetPoints()->SetPoint(1, 1.0, -1.0, -1.0);
  aCube->GetPoints()->SetPoint(2, 1.0, 1.0, -1.0);
  aCube->GetPoints()->SetPoint(3, -1.0, 1.0, -1.0);
  aCube->GetPoints()->SetPoint(4, -1.0, -1.0, 1.0);
  aCube->GetPoints()->SetPoint(5, 1.0, -1.0, 1.0);
  aCube->GetPoints()->SetPoint(6, 1.0, 1.0, 1.0);
  aCube->GetPoints()->SetPoint(7, -1.0, 1.0, 1.0);

  svtkIdType faces[31] = {
    6,             // number of faces
    4, 0, 3, 2, 1, //
    4, 0, 4, 7, 3, //
    4, 4, 5, 6, 7, //
    4, 5, 1, 2, 6, //
    4, 0, 1, 5, 4, //
    4, 2, 3, 7, 6  //
  };

  aCube->SetFaces(faces);
  aCube->Initialize();
  return aCube;
}

svtkSmartPointer<svtkPolyhedron> MakeDodecahedron()
{
  svtkSmartPointer<svtkPolyhedron> aDodecahedron = svtkSmartPointer<svtkPolyhedron>::New();

  // create polyhedron (dodecahedron)
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  for (int i = 0; i < 20; ++i)
  {
    aDodecahedron->GetPointIds()->InsertNextId(i);
  }

  aDodecahedron->GetPoints()->InsertNextPoint(1.21412, 0, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, 1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, 0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, -0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, -1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(1.96449, 0, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, 1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, 1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, -1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, -1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, 1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, 1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.96449, 0, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, -1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, -1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, 0.713644, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, 1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.21412, 0, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, -1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, -0.713644, -1.58931);

  svtkIdType faces[73] = {
    12,                   // number of faces
    5, 0, 1, 2, 3, 4,     // number of ids on face, ids
    5, 0, 5, 10, 6, 1,    //
    5, 1, 6, 11, 7, 2,    //
    5, 2, 7, 12, 8, 3,    //
    5, 3, 8, 13, 9, 4,    //
    5, 4, 9, 14, 5, 0,    //
    5, 15, 10, 5, 14, 19, //
    5, 16, 11, 6, 10, 15, //
    5, 17, 12, 7, 11, 16, //
    5, 18, 13, 8, 12, 17, //
    5, 19, 14, 9, 13, 18, //
    5, 19, 18, 17, 16, 15 //
  };

  aDodecahedron->SetFaces(faces);
  aDodecahedron->Initialize();

  return aDodecahedron;
}

svtkSmartPointer<svtkPentagonalPrism> MakePentagonalPrism()
{
  svtkSmartPointer<svtkPentagonalPrism> aPentagonalPrism = svtkSmartPointer<svtkPentagonalPrism>::New();

  aPentagonalPrism->GetPointIds()->SetId(0, 0);
  aPentagonalPrism->GetPointIds()->SetId(1, 1);
  aPentagonalPrism->GetPointIds()->SetId(2, 2);
  aPentagonalPrism->GetPointIds()->SetId(3, 3);
  aPentagonalPrism->GetPointIds()->SetId(4, 4);
  aPentagonalPrism->GetPointIds()->SetId(5, 5);
  aPentagonalPrism->GetPointIds()->SetId(6, 6);
  aPentagonalPrism->GetPointIds()->SetId(7, 7);
  aPentagonalPrism->GetPointIds()->SetId(8, 8);
  aPentagonalPrism->GetPointIds()->SetId(9, 9);

  aPentagonalPrism->GetPoints()->SetPoint(0, 11, 10, 10);
  aPentagonalPrism->GetPoints()->SetPoint(1, 13, 10, 10);
  aPentagonalPrism->GetPoints()->SetPoint(2, 14, 12, 10);
  aPentagonalPrism->GetPoints()->SetPoint(3, 12, 14, 10);
  aPentagonalPrism->GetPoints()->SetPoint(4, 10, 12, 10);
  aPentagonalPrism->GetPoints()->SetPoint(5, 11, 10, 14);
  aPentagonalPrism->GetPoints()->SetPoint(6, 13, 10, 14);
  aPentagonalPrism->GetPoints()->SetPoint(7, 14, 12, 14);
  aPentagonalPrism->GetPoints()->SetPoint(8, 12, 14, 14);
  aPentagonalPrism->GetPoints()->SetPoint(9, 10, 12, 14);

  return aPentagonalPrism;
}

svtkSmartPointer<svtkHexagonalPrism> MakeHexagonalPrism()
{
  svtkSmartPointer<svtkHexagonalPrism> aHexagonalPrism = svtkSmartPointer<svtkHexagonalPrism>::New();
  aHexagonalPrism->GetPointIds()->SetId(0, 0);
  aHexagonalPrism->GetPointIds()->SetId(1, 1);
  aHexagonalPrism->GetPointIds()->SetId(2, 2);
  aHexagonalPrism->GetPointIds()->SetId(3, 3);
  aHexagonalPrism->GetPointIds()->SetId(4, 4);
  aHexagonalPrism->GetPointIds()->SetId(5, 5);
  aHexagonalPrism->GetPointIds()->SetId(6, 6);
  aHexagonalPrism->GetPointIds()->SetId(7, 7);
  aHexagonalPrism->GetPointIds()->SetId(8, 8);
  aHexagonalPrism->GetPointIds()->SetId(9, 9);
  aHexagonalPrism->GetPointIds()->SetId(10, 10);
  aHexagonalPrism->GetPointIds()->SetId(11, 11);

  aHexagonalPrism->GetPoints()->SetPoint(0, 11, 10, 10);
  aHexagonalPrism->GetPoints()->SetPoint(1, 13, 10, 10);
  aHexagonalPrism->GetPoints()->SetPoint(2, 14, 12, 10);
  aHexagonalPrism->GetPoints()->SetPoint(3, 13, 14, 10);
  aHexagonalPrism->GetPoints()->SetPoint(4, 11, 14, 10);
  aHexagonalPrism->GetPoints()->SetPoint(5, 10, 12, 10);
  aHexagonalPrism->GetPoints()->SetPoint(6, 11, 10, 14);
  aHexagonalPrism->GetPoints()->SetPoint(7, 13, 10, 14);
  aHexagonalPrism->GetPoints()->SetPoint(8, 14, 12, 14);
  aHexagonalPrism->GetPoints()->SetPoint(9, 13, 14, 14);
  aHexagonalPrism->GetPoints()->SetPoint(10, 11, 14, 14);
  aHexagonalPrism->GetPoints()->SetPoint(11, 10, 12, 14);

  return aHexagonalPrism;
}

svtkSmartPointer<svtkLagrangeCurve> MakeLagrangeCurve()
{
  int nPoints = 5;

  svtkSmartPointer<svtkLagrangeCurve> curve = svtkSmartPointer<svtkLagrangeCurve>::New();

  curve->GetPointIds()->SetNumberOfIds(nPoints);
  curve->GetPoints()->SetNumberOfPoints(nPoints);
  curve->Initialize();
  double* points = curve->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    curve->GetPointIds()->SetId(i, i);
    curve->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return curve;
}

svtkSmartPointer<svtkLagrangeTriangle> MakeLagrangeTriangle()
{
  int nPoints = 15;

  svtkSmartPointer<svtkLagrangeTriangle> triangle = svtkSmartPointer<svtkLagrangeTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, i);
    triangle->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return triangle;
}

svtkSmartPointer<svtkLagrangeTriangle> MakeBrokenLagrangeTriangle()
{
  int nPoints = 6;

  svtkSmartPointer<svtkLagrangeTriangle> triangle = svtkSmartPointer<svtkLagrangeTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, (i == 2 ? 1 : i == 1 ? 2 : i));
    triangle->GetPoints()->SetPoint(i, &points[3 * (i == 2 ? 1 : i == 1 ? 2 : i)]);
  }

  return triangle;
}

svtkSmartPointer<svtkLagrangeQuadrilateral> MakeLagrangeQuadrilateral()
{
  int nPoints = 25;

  svtkSmartPointer<svtkLagrangeQuadrilateral> quadrilateral =
    svtkSmartPointer<svtkLagrangeQuadrilateral>::New();

  quadrilateral->GetPointIds()->SetNumberOfIds(nPoints);
  quadrilateral->GetPoints()->SetNumberOfPoints(nPoints);
  quadrilateral->SetUniformOrderFromNumPoints(nPoints);
  quadrilateral->Initialize();
  double* points = quadrilateral->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    quadrilateral->GetPointIds()->SetId(i, i);
    quadrilateral->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return quadrilateral;
}

svtkSmartPointer<svtkLagrangeHexahedron> MakeLagrangeHexahedron()
{
  int nPoints = 125;

  svtkSmartPointer<svtkLagrangeHexahedron> hexahedron = svtkSmartPointer<svtkLagrangeHexahedron>::New();

  hexahedron->GetPointIds()->SetNumberOfIds(nPoints);
  hexahedron->GetPoints()->SetNumberOfPoints(nPoints);
  hexahedron->SetUniformOrderFromNumPoints(nPoints);
  hexahedron->Initialize();
  double* points = hexahedron->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    hexahedron->GetPointIds()->SetId(i, i);
    hexahedron->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return hexahedron;
}

svtkSmartPointer<svtkLagrangeTetra> MakeLagrangeTetra()
{
  int nPoints = 10;

  svtkSmartPointer<svtkLagrangeTetra> tetra = svtkSmartPointer<svtkLagrangeTetra>::New();

  tetra->GetPointIds()->SetNumberOfIds(nPoints);
  tetra->GetPoints()->SetNumberOfPoints(nPoints);
  tetra->Initialize();
  double* points = tetra->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    tetra->GetPointIds()->SetId(i, i);
    tetra->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return tetra;
}

svtkSmartPointer<svtkLagrangeWedge> MakeLagrangeWedge()
{
  int nPoints = 75;

  svtkSmartPointer<svtkLagrangeWedge> wedge = svtkSmartPointer<svtkLagrangeWedge>::New();

  wedge->GetPointIds()->SetNumberOfIds(nPoints);
  wedge->GetPoints()->SetNumberOfPoints(nPoints);
  wedge->SetUniformOrderFromNumPoints(nPoints);
  wedge->Initialize();
  double* points = wedge->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return wedge;
}

svtkSmartPointer<svtkBezierCurve> MakeBezierCurve()
{
  int nPoints = 5;

  svtkSmartPointer<svtkBezierCurve> curve = svtkSmartPointer<svtkBezierCurve>::New();

  curve->GetPointIds()->SetNumberOfIds(nPoints);
  curve->GetPoints()->SetNumberOfPoints(nPoints);
  curve->Initialize();
  double* points = curve->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    curve->GetPointIds()->SetId(i, i);
    curve->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return curve;
}

svtkSmartPointer<svtkBezierTriangle> MakeBezierTriangle()
{
  int nPoints = 15;

  svtkSmartPointer<svtkBezierTriangle> triangle = svtkSmartPointer<svtkBezierTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, i);
    triangle->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return triangle;
}

svtkSmartPointer<svtkBezierQuadrilateral> MakeBezierQuadrilateral()
{
  int nPoints = 25;

  svtkSmartPointer<svtkBezierQuadrilateral> quadrilateral =
    svtkSmartPointer<svtkBezierQuadrilateral>::New();

  quadrilateral->GetPointIds()->SetNumberOfIds(nPoints);
  quadrilateral->GetPoints()->SetNumberOfPoints(nPoints);
  quadrilateral->SetUniformOrderFromNumPoints(nPoints);
  quadrilateral->Initialize();
  double* points = quadrilateral->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    quadrilateral->GetPointIds()->SetId(i, i);
    quadrilateral->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return quadrilateral;
}

svtkSmartPointer<svtkBezierHexahedron> MakeBezierHexahedron()
{
  int nPoints = 125;

  svtkSmartPointer<svtkBezierHexahedron> hexahedron = svtkSmartPointer<svtkBezierHexahedron>::New();

  hexahedron->GetPointIds()->SetNumberOfIds(nPoints);
  hexahedron->GetPoints()->SetNumberOfPoints(nPoints);
  hexahedron->SetUniformOrderFromNumPoints(nPoints);
  hexahedron->Initialize();
  double* points = hexahedron->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    hexahedron->GetPointIds()->SetId(i, i);
    hexahedron->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return hexahedron;
}

svtkSmartPointer<svtkBezierTetra> MakeBezierTetra()
{
  int nPoints = 10;

  svtkSmartPointer<svtkBezierTetra> tetra = svtkSmartPointer<svtkBezierTetra>::New();

  tetra->GetPointIds()->SetNumberOfIds(nPoints);
  tetra->GetPoints()->SetNumberOfPoints(nPoints);
  tetra->Initialize();
  double* points = tetra->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    tetra->GetPointIds()->SetId(i, i);
    tetra->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return tetra;
}

svtkSmartPointer<svtkBezierWedge> MakeBezierWedge()
{
  int nPoints = 75;

  svtkSmartPointer<svtkBezierWedge> wedge = svtkSmartPointer<svtkBezierWedge>::New();

  wedge->GetPointIds()->SetNumberOfIds(nPoints);
  wedge->GetPoints()->SetNumberOfPoints(nPoints);
  wedge->SetUniformOrderFromNumPoints(nPoints);
  wedge->Initialize();
  double* points = wedge->GetParametricCoords();
  for (svtkIdType i = 0; i < nPoints; i++)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return wedge;
}
