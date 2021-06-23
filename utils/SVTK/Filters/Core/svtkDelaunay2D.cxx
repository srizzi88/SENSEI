/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDelaunay2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDelaunay2D.h"

#include "svtkAbstractTransform.h"
#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"
#include "svtkTriangle.h"

#include <set>
#include <vector>

svtkStandardNewMacro(svtkDelaunay2D);
svtkCxxSetObjectMacro(svtkDelaunay2D, Transform, svtkAbstractTransform);

// Construct object with Alpha = 0.0; Tolerance = 0.00001; Offset = 1.25;
// BoundingTriangulation turned off.
svtkDelaunay2D::svtkDelaunay2D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.00001;
  this->BoundingTriangulation = 0;
  this->Offset = 1.0;
  this->Transform = nullptr;
  this->ProjectionPlaneMode = SVTK_DELAUNAY_XY_PLANE;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);
}

svtkDelaunay2D::~svtkDelaunay2D()
{
  if (this->Transform)
  {
    this->Transform->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
void svtkDelaunay2D::SetSourceData(svtkPolyData* input)
{
  this->Superclass::SetInputData(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter. New style.
void svtkDelaunay2D::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

svtkPolyData* svtkDelaunay2D::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
// (Note that z-component is ignored.)
int svtkDelaunay2D::InCircle(double x[3], double x1[3], double x2[3], double x3[3])
{
  double radius2, center[2], dist2;

  radius2 = svtkTriangle::Circumcircle(x1, x2, x3, center);

  // check if inside/outside circumcircle
  dist2 = (x[0] - center[0]) * (x[0] - center[0]) + (x[1] - center[1]) * (x[1] - center[1]);

  if (dist2 < (0.999999999999 * radius2))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

#define SVTK_DEL2D_TOLERANCE 1.0e-014

// Recursive method to locate triangle containing point. Starts with arbitrary
// triangle (tri) and "walks" towards it. Influenced by some of Guibas and
// Stolfi's work. Returns id of enclosing triangle, or -1 if no triangle
// found. Also, the array nei[3] is used to communicate info about points
// that lie on triangle edges: nei[0] is neighboring triangle id, and nei[1]
// and nei[2] are the vertices defining the edge.
svtkIdType svtkDelaunay2D::FindTriangle(double x[3], svtkIdType ptIds[3], svtkIdType tri, double tol,
  svtkIdType nei[3], svtkIdList* neighbors)
{
  int i, j, ir, ic, inside, i2, i3;
  const svtkIdType* pts;
  svtkIdType npts;
  svtkIdType newNei;
  double p[3][3], n[2], vp[2], vx[2], dp, minProj;

  // get local triangle info
  this->Mesh->GetCellPoints(tri, npts, pts);
  for (i = 0; i < 3; i++)
  {
    ptIds[i] = pts[i];
    this->GetPoint(ptIds[i], p[i]);
  }

  // Randomization (of find edge neighbora) avoids walking in
  // circles in certain weird cases
  srand(tri);
  ir = rand() % 3;
  // evaluate in/out of each edge
  for (inside = 1, minProj = SVTK_DEL2D_TOLERANCE, ic = 0; ic < 3; ic++)
  {
    i = (ir + ic) % 3;
    i2 = (i + 1) % 3;
    i3 = (i + 2) % 3;

    // create a 2D edge normal to define a "half-space"; evaluate points (i.e.,
    // candidate point and other triangle vertex not on this edge).
    n[0] = -(p[i2][1] - p[i][1]);
    n[1] = p[i2][0] - p[i][0];
    svtkMath::Normalize2D(n);

    // compute local vectors
    for (j = 0; j < 2; j++)
    {
      vp[j] = p[i3][j] - p[i][j];
      vx[j] = x[j] - p[i][j];
    }

    // check for duplicate point
    svtkMath::Normalize2D(vp);
    if (svtkMath::Normalize2D(vx) <= tol)
    {
      this->NumberOfDuplicatePoints++;
      return -1;
    }

    // see if two points are in opposite half spaces
    dp = svtkMath::Dot2D(n, vx) * (svtkMath::Dot2D(n, vp) < 0 ? -1.0 : 1.0);
    if (dp < SVTK_DEL2D_TOLERANCE)
    {
      if (dp < minProj) // track edge most orthogonal to point direction
      {
        inside = 0;
        nei[1] = ptIds[i];
        nei[2] = ptIds[i2];
        minProj = dp;
      }
    } // outside this edge
  }   // for each edge

  if (inside) // all edges have tested positive
  {
    nei[0] = (-1);
    return tri;
  }

  else if (!inside && (fabs(minProj) < SVTK_DEL2D_TOLERANCE)) // on edge
  {
    this->Mesh->GetCellEdgeNeighbors(tri, nei[1], nei[2], neighbors);
    nei[0] = neighbors->GetId(0);
    return tri;
  }

  else // walk towards point
  {
    this->Mesh->GetCellEdgeNeighbors(tri, nei[1], nei[2], neighbors);
    if ((newNei = neighbors->GetId(0)) == nei[0])
    {
      this->NumberOfDegeneracies++;
      return -1;
    }
    else
    {
      nei[0] = tri;
      return this->FindTriangle(x, ptIds, newNei, tol, nei, neighbors);
    }
  }
}

#undef SVTK_DEL2D_TOLERANCE

// Recursive method checks whether edge is Delaunay, and if not, swaps edge.
// Continues until all edges are Delaunay. Points p1 and p2 form the edge in
// question; x is the coordinates of the inserted point; tri is the current
// triangle id.
void svtkDelaunay2D::CheckEdge(
  svtkIdType ptId, double x[3], svtkIdType p1, svtkIdType p2, svtkIdType tri, bool recursive)
{
  int i;
  const svtkIdType* pts;
  svtkIdType npts;
  svtkIdType numNei, nei, p3;
  double x1[3], x2[3], x3[3];
  svtkIdList* neighbors;
  svtkIdType swapTri[3];

  this->GetPoint(p1, x1);
  this->GetPoint(p2, x2);

  neighbors = svtkIdList::New();
  neighbors->Allocate(2);

  this->Mesh->GetCellEdgeNeighbors(tri, p1, p2, neighbors);
  numNei = neighbors->GetNumberOfIds();

  if (numNei > 0) // i.e., not a boundary edge
  {
    // get neighbor info including opposite point
    nei = neighbors->GetId(0);
    this->Mesh->GetCellPoints(nei, npts, pts);
    for (i = 0; i < 2; i++)
    {
      if (pts[i] != p1 && pts[i] != p2)
      {
        break;
      }
    }
    p3 = pts[i];
    this->GetPoint(p3, x3);

    // see whether point is in circumcircle
    if (this->InCircle(x3, x, x1, x2))
    { // swap diagonal
      this->Mesh->RemoveReferenceToCell(p1, tri);
      this->Mesh->RemoveReferenceToCell(p2, nei);
      this->Mesh->ResizeCellList(ptId, 1);
      this->Mesh->AddReferenceToCell(ptId, nei);
      this->Mesh->ResizeCellList(p3, 1);
      this->Mesh->AddReferenceToCell(p3, tri);

      swapTri[0] = ptId;
      swapTri[1] = p3;
      swapTri[2] = p2;
      this->Mesh->ReplaceCell(tri, 3, swapTri);

      swapTri[0] = ptId;
      swapTri[1] = p1;
      swapTri[2] = p3;
      this->Mesh->ReplaceCell(nei, 3, swapTri);

      if (recursive)
      {
        // two new edges become suspect
        this->CheckEdge(ptId, x, p3, p2, tri, true);
        this->CheckEdge(ptId, x, p1, p3, nei, true);
      }
    } // in circle
  }   // interior edge

  neighbors->Delete();
}

// 2D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find triangle point is in
//   3. Create 3 triangles from each edge of triangle that point is in
//   4. Recursively evaluate Delaunay criterion for each edge neighbor
//   5. If criterion not satisfied; swap diagonal
//
int svtkDelaunay2D::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* source = nullptr;
  if (sourceInfo)
  {
    source = svtkPolyData::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPoints, i;
  svtkIdType numTriangles = 0;
  svtkIdType ptId, tri[4], nei[3];
  svtkIdType p1 = 0;
  svtkIdType p2 = 0;
  svtkIdType p3 = 0;
  svtkPoints* inPoints;
  svtkPoints* points;
  svtkPoints* tPoints = nullptr;
  svtkCellArray* triangles;
  int ncells;
  svtkIdType nodes[4][3];
  const svtkIdType* neiPts;
  const svtkIdType* triPts = nullptr;
  svtkIdType numNeiPts;
  svtkIdType npts = 0;
  svtkIdType pts[3], swapPts[3];
  svtkIdList *neighbors, *cells;
  svtkIdType tri1, tri2;
  double center[3], radius, tol, x[3];
  double n1[3], n2[3];
  int* triUse = nullptr;

  svtkDebugMacro(<< "Generating 2D Delaunay triangulation");

  if (this->Transform && this->BoundingTriangulation)
  {
    svtkWarningMacro(<< "Bounding triangulation cannot be used when an input transform is "
                       "specified.  Output will not contain bounding triangulation.");
  }

  if (this->ProjectionPlaneMode == SVTK_BEST_FITTING_PLANE && this->BoundingTriangulation)
  {
    svtkWarningMacro(<< "Bounding triangulation cannot be used when the best fitting plane option "
                       "is on.  Output will not contain bounding triangulation.");
  }

  // Initialize; check input
  //
  if ((inPoints = input->GetPoints()) == nullptr)
  {
    svtkDebugMacro("Cannot triangulate; no input points");
    return 1;
  }

  if ((numPoints = inPoints->GetNumberOfPoints()) <= 2)
  {
    svtkDebugMacro("Cannot triangulate; need at least 3 input points");
    return 1;
  }

  neighbors = svtkIdList::New();
  neighbors->Allocate(2);
  cells = svtkIdList::New();
  cells->Allocate(64);

  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  this->Mesh = svtkPolyData::New();

  // If the user specified a transform, apply it to the input data.
  //
  // Only the input points are transformed.  We do not bother
  // transforming the source points (if specified).  The reason is
  // that only the topology of the Source is used during the constrain
  // operation.  The point ids in the Source topology are assumed to
  // reference points in the input. So, when an input transform is
  // used, only the input points are transformed.  We do not bother
  // with transforming the Source points since they are never
  // referenced.
  if (this->Transform)
  {
    tPoints = svtkPoints::New();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else
  {
    // If the user asked this filter to compute the best fitting plane,
    // proceed to compute the plane and generate a transform that will
    // map the input points into that plane.
    if (this->ProjectionPlaneMode == SVTK_BEST_FITTING_PLANE)
    {
      this->SetTransform(this->ComputeBestFittingPlane(input));
      tPoints = svtkPoints::New();
      this->Transform->TransformPoints(inPoints, tPoints);
    }
  }

  // Create initial bounding triangulation. Have to create bounding points.
  // Initialize mesh structure.
  //
  points = svtkPoints::New();
  // This will copy doubles to doubles if the input is double.
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPoints);
  if (!this->Transform)
  {
    points->DeepCopy(inPoints);
  }
  else
  {
    points->DeepCopy(tPoints);
    tPoints->Delete();
    tPoints = nullptr;
  }

  const double* bounds = points->GetBounds();
  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;
  tol = input->GetLength();
  radius = this->Offset * tol;
  tol *= this->Tolerance;

  for (ptId = 0; ptId < 8; ptId++)
  {
    x[0] = center[0] + radius * cos(ptId * svtkMath::RadiansFromDegrees(45.0));
    x[1] = center[1] + radius * sin(ptId * svtkMath::RadiansFromDegrees(45.0));
    x[2] = center[2];
    points->InsertPoint(numPoints + ptId, x);
  }
  // We do this for speed accessing points
  this->Points = static_cast<svtkDoubleArray*>(points->GetData())->GetPointer(0);

  triangles = svtkCellArray::New();
  triangles->AllocateEstimate(2 * numPoints, 3);

  // create bounding triangles (there are six)
  pts[0] = numPoints;
  pts[1] = numPoints + 1;
  pts[2] = numPoints + 2;
  triangles->InsertNextCell(3, pts);
  pts[0] = numPoints + 2;
  pts[1] = numPoints + 3;
  pts[2] = numPoints + 4;
  triangles->InsertNextCell(3, pts);
  pts[0] = numPoints + 4;
  pts[1] = numPoints + 5;
  pts[2] = numPoints + 6;
  triangles->InsertNextCell(3, pts);
  pts[0] = numPoints + 6;
  pts[1] = numPoints + 7;
  pts[2] = numPoints + 0;
  triangles->InsertNextCell(3, pts);
  pts[0] = numPoints + 0;
  pts[1] = numPoints + 2;
  pts[2] = numPoints + 6;
  triangles->InsertNextCell(3, pts);
  pts[0] = numPoints + 2;
  pts[1] = numPoints + 4;
  pts[2] = numPoints + 6;
  triangles->InsertNextCell(3, pts);
  tri[0] = 0;

  this->Mesh->SetPoints(points);
  this->Mesh->SetPolys(triangles);
  this->Mesh->BuildLinks(); // build cell structure

  // For each point; find triangle containing point. Then evaluate three
  // neighboring triangles for Delaunay criterion. Triangles that do not
  // satisfy criterion have their edges swapped. This continues recursively
  // until all triangles have been shown to be Delaunay.
  //
  for (ptId = 0; ptId < numPoints; ptId++)
  {
    this->GetPoint(ptId, x);
    nei[0] = (-1); // where we are coming from...nowhere initially

    if ((tri[0] = this->FindTriangle(x, pts, tri[0], tol, nei, neighbors)) >= 0)
    {
      if (nei[0] < 0) // in triangle
      {
        // delete this triangle; create three new triangles
        // first triangle is replaced with one of the new ones
        nodes[0][0] = ptId;
        nodes[0][1] = pts[0];
        nodes[0][2] = pts[1];
        this->Mesh->RemoveReferenceToCell(pts[2], tri[0]);
        this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        this->Mesh->ResizeCellList(ptId, 1);
        this->Mesh->AddReferenceToCell(ptId, tri[0]);

        // create two new triangles
        nodes[1][0] = ptId;
        nodes[1][1] = pts[1];
        nodes[1][2] = pts[2];
        tri[1] = this->Mesh->InsertNextLinkedCell(SVTK_TRIANGLE, 3, nodes[1]);

        nodes[2][0] = ptId;
        nodes[2][1] = pts[2];
        nodes[2][2] = pts[0];
        tri[2] = this->Mesh->InsertNextLinkedCell(SVTK_TRIANGLE, 3, nodes[2]);

        // Check edge neighbors for Delaunay criterion. If not satisfied, flip
        // edge diagonal. (This is done recursively.)
        this->CheckEdge(ptId, x, pts[0], pts[1], tri[0], true);
        this->CheckEdge(ptId, x, pts[1], pts[2], tri[1], true);
        this->CheckEdge(ptId, x, pts[2], pts[0], tri[2], true);
      }

      else // on triangle edge
      {
        // update cell list
        this->Mesh->GetCellPoints(nei[0], numNeiPts, neiPts);
        for (i = 0; i < 3; i++)
        {
          if (neiPts[i] != nei[1] && neiPts[i] != nei[2])
          {
            p1 = neiPts[i];
          }
          if (pts[i] != nei[1] && pts[i] != nei[2])
          {
            p2 = pts[i];
          }
        }
        this->Mesh->ResizeCellList(p1, 1);
        this->Mesh->ResizeCellList(p2, 1);

        // replace two triangles
        this->Mesh->RemoveReferenceToCell(nei[2], tri[0]);
        this->Mesh->RemoveReferenceToCell(nei[2], nei[0]);
        nodes[0][0] = ptId;
        nodes[0][1] = p2;
        nodes[0][2] = nei[1];
        this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        nodes[1][0] = ptId;
        nodes[1][1] = p1;
        nodes[1][2] = nei[1];
        this->Mesh->ReplaceCell(nei[0], 3, nodes[1]);
        this->Mesh->ResizeCellList(ptId, 2);
        this->Mesh->AddReferenceToCell(ptId, tri[0]);
        this->Mesh->AddReferenceToCell(ptId, nei[0]);

        tri[1] = nei[0];

        // create two new triangles
        nodes[2][0] = ptId;
        nodes[2][1] = p2;
        nodes[2][2] = nei[2];
        tri[2] = this->Mesh->InsertNextLinkedCell(SVTK_TRIANGLE, 3, nodes[2]);

        nodes[3][0] = ptId;
        nodes[3][1] = p1;
        nodes[3][2] = nei[2];
        tri[3] = this->Mesh->InsertNextLinkedCell(SVTK_TRIANGLE, 3, nodes[3]);

        // Check edge neighbors for Delaunay criterion.
        for (i = 0; i < 4; i++)
        {
          this->CheckEdge(ptId, x, nodes[i][1], nodes[i][2], tri[i], true);
        }
      }
    } // if triangle found

    else
    {
      tri[0] = 0; // no triangle found
    }

    if (!(ptId % 1000))
    {
      svtkDebugMacro(<< "point #" << ptId);
      this->UpdateProgress(static_cast<double>(ptId) / numPoints);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

  } // for all points

  svtkDebugMacro(<< "Triangulated " << numPoints << " points, " << this->NumberOfDuplicatePoints
                << " of which were duplicates");

  if (this->NumberOfDegeneracies > 0)
  {
    svtkDebugMacro(<< this->NumberOfDegeneracies
                  << " degenerate triangles encountered, mesh quality suspect");
  }

  // Finish up by recovering the boundary, or deleting all triangles connected
  // to the bounding triangulation points or not satisfying alpha criterion,
  if (!this->BoundingTriangulation || this->Alpha > 0.0 || source)
  {
    numTriangles = this->Mesh->GetNumberOfCells();
    if (source)
    {
      triUse = this->RecoverBoundary(source);
    }
    else
    {
      triUse = new int[numTriangles];
      for (i = 0; i < numTriangles; i++)
      {
        triUse[i] = 1;
      }
    }
  }

  // Delete triangles connected to boundary points (if not desired)
  if (!this->BoundingTriangulation)
  {
    for (ptId = numPoints; ptId < (numPoints + 8); ptId++)
    {
      this->Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();
      for (i = 0; i < ncells; i++)
      {
        triUse[cells->GetId(i)] = 0; // mark as deleted
      }
    }
  }

  // If non-zero alpha value, then figure out which parts of mesh are
  // contained within alpha radius.
  //
  if (this->Alpha > 0.0)
  {
    double alpha2 = this->Alpha * this->Alpha;
    double x1[3], x2[3], x3[3];
    double xx1[3], xx2[3], xx3[3];
    svtkIdType cellId, numNei, ap1, ap2, neighbor;

    svtkCellArray* alphaVerts = svtkCellArray::New();
    alphaVerts->AllocateEstimate(numPoints, 1);
    svtkCellArray* alphaLines = svtkCellArray::New();
    alphaLines->AllocateEstimate(numPoints, 2);

    char* pointUse = new char[numPoints + 8];
    for (ptId = 0; ptId < (numPoints + 8); ptId++)
    {
      pointUse[ptId] = 0;
    }

    // traverse all triangles; evaluating Delaunay criterion
    for (i = 0; i < numTriangles; i++)
    {
      if (triUse[i] == 1)
      {
        this->Mesh->GetCellPoints(i, npts, triPts);

        // if any point is one of the bounding points that was added
        // at the beginning of the algorithm, then grab the points
        // from the variable "points" (this list has the boundary
        // points and the original points have been transformed by the
        // input transform).  if none of the points are bounding points,
        // then grab the points from the variable "inPoints" so the alpha
        // criterion is applied in the nontransformed space.
        if (triPts[0] < numPoints && triPts[1] < numPoints && triPts[2] < numPoints)
        {
          inPoints->GetPoint(triPts[0], x1);
          inPoints->GetPoint(triPts[1], x2);
          inPoints->GetPoint(triPts[2], x3);
        }
        else
        {
          points->GetPoint(triPts[0], x1);
          points->GetPoint(triPts[1], x2);
          points->GetPoint(triPts[2], x3);
        }

        // evaluate the alpha criterion in 3D
        svtkTriangle::ProjectTo2D(x1, x2, x3, xx1, xx2, xx3);
        if (svtkTriangle::Circumcircle(xx1, xx2, xx3, center) > alpha2)
        {
          triUse[i] = 0;
        }
        else
        {
          for (int j = 0; j < 3; j++)
          {
            pointUse[triPts[j]] = 1;
          }
        }
      } // if non-deleted triangle
    }   // for all triangles

    // traverse all edges see whether we need to create some
    for (cellId = 0, triangles->InitTraversal(); triangles->GetNextCell(npts, triPts); cellId++)
    {
      if (!triUse[cellId])
      {
        for (i = 0; i < npts; i++)
        {
          ap1 = triPts[i];
          ap2 = triPts[(i + 1) % npts];

          if (this->BoundingTriangulation || (ap1 < numPoints && ap2 < numPoints))
          {
            this->Mesh->GetCellEdgeNeighbors(cellId, ap1, ap2, neighbors);
            numNei = neighbors->GetNumberOfIds();

            if (numNei < 1 || ((neighbor = neighbors->GetId(0)) > cellId && !triUse[neighbor]))
            { // see whether edge is shorter than Alpha

              // same argument as above, if one is a boundary point, get
              // it using this->GetPoint() which are transformed points. if
              // neither of the points are boundary points, get the from
              // inPoints (untransformed points) so alpha comparison done
              // untransformed space
              if (ap1 < numPoints && ap2 < numPoints)
              {
                inPoints->GetPoint(ap1, x1);
                inPoints->GetPoint(ap2, x2);
              }
              else
              {
                this->GetPoint(ap1, x1);
                this->GetPoint(ap2, x2);
              }
              if ((svtkMath::Distance2BetweenPoints(x1, x2) * 0.25) <= alpha2)
              {
                pointUse[ap1] = 1;
                pointUse[ap2] = 1;
                pts[0] = ap1;
                pts[1] = ap2;
                alphaLines->InsertNextCell(2, pts);
              } // if passed test
            }   // test edge
          }     // if valid edge
        }       // for all edges of this triangle
      }         // if triangle not output
    }           // for all triangles

    // traverse all points, create vertices if none used
    for (ptId = 0; ptId < (numPoints + 8); ptId++)
    {
      if ((ptId < numPoints || this->BoundingTriangulation) && !pointUse[ptId])
      {
        pts[0] = ptId;
        alphaVerts->InsertNextCell(1, pts);
      }
    }

    // update output
    delete[] pointUse;
    output->SetVerts(alphaVerts);
    alphaVerts->Delete();
    output->SetLines(alphaLines);
    alphaLines->Delete();
  }

  // The code below fixes a bug reported by Gilles Rougeron.
  // Some input points were not connected in the output triangulation.
  // The cause was that those points were only connected to triangles
  // scheduled for removal (i.e. triangles connected to the boundary).
  //
  // We wrote the following fix: swap edges so the unconnected points
  // become connected to new triangles not scheduled for removal.
  // We only applies if:
  // - the bounding triangulation must be deleted
  //   (BoundingTriangulation == OFF)
  // - alpha spheres are not used (Alpha == 0.0)
  // - the triangulation is not constrained (source == nullptr)

  if (!this->BoundingTriangulation && this->Alpha == 0.0 && !source)
  {
    bool isConnected;
    svtkIdType numSwaps = 0;

    for (ptId = 0; ptId < numPoints; ptId++)
    {
      // check if point is only connected to triangles scheduled for
      // removal
      this->Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();

      isConnected = false;

      for (i = 0; i < ncells; i++)
      {
        if (triUse[cells->GetId(i)])
        {
          isConnected = true;
          break;
        }
      }

      // this point will be connected in the output
      if (isConnected)
      {
        // point is connected: continue
        continue;
      }

      // This point is only connected to triangles scheduled for removal.
      // Therefore it will not be connected in the output triangulation.
      // Let's swap edges to create a triangle with 3 inner points.
      // - inner points have an id < numPoints
      // - boundary point ids are, numPoints <= id < numPoints+8.

      // visit every edge connected to that point.
      // check the 2 triangles touching at that edge.
      // if one triangle is connected to 2 non-boundary points

      for (i = 0; i < ncells; i++)
      {
        tri1 = cells->GetId(i);
        this->Mesh->GetCellPoints(tri1, npts, triPts);

        if (triPts[0] == ptId)
        {
          p1 = triPts[1];
          p2 = triPts[2];
        }
        else if (triPts[1] == ptId)
        {
          p1 = triPts[2];
          p2 = triPts[0];
        }
        else
        {
          p1 = triPts[0];
          p2 = triPts[1];
        }

        // if both p1 & p2 are boundary points,
        // we skip them.
        if (p1 >= numPoints && p2 >= numPoints)
        {
          continue;
        }

        svtkDebugMacro(
          "tri " << tri1 << " [" << triPts[0] << " " << triPts[1] << " " << triPts[2] << "]");

        svtkDebugMacro("edge [" << p1 << " " << p2 << "] non-boundary");

        // get the triangle sharing edge [p1 p2] with tri1
        this->Mesh->GetCellEdgeNeighbors(tri1, p1, p2, neighbors);

        // Since p1 or p2 is not on the boundary,
        // the neighbor triangle should exist.
        // If more than one neighbor triangle exist,
        // the edge is non-manifold.
        if (neighbors->GetNumberOfIds() != 1)
        {
          svtkErrorMacro("ERROR: Edge [" << p1 << " " << p2 << "] is non-manifold!!!");
          return 0;
        }

        tri2 = neighbors->GetId(0);

        // get the 3 points of the neighbor triangle
        this->Mesh->GetCellPoints(tri2, npts, neiPts);

        svtkDebugMacro(
          "triangle " << tri2 << " [" << neiPts[0] << " " << neiPts[1] << " " << neiPts[2] << "]");

        // locate the point different from p1 and p2
        if (neiPts[0] != p1 && neiPts[0] != p2)
        {
          p3 = neiPts[0];
        }
        else if (neiPts[1] != p1 && neiPts[1] != p2)
        {
          p3 = neiPts[1];
        }
        else
        {
          p3 = neiPts[2];
        }

        svtkDebugMacro("swap [" << p1 << " " << p2 << "] and [" << ptId << " " << p3 << "]");

        // create the two new triangles.
        // we just need to replace their pt ids.
        pts[0] = ptId;
        pts[1] = p1;
        pts[2] = p3;
        swapPts[0] = ptId;
        swapPts[1] = p3;
        swapPts[2] = p2;

        svtkDebugMacro("candidate tri1 " << tri1 << " [" << pts[0] << " " << pts[1] << " " << pts[2]
                                        << "]"
                                        << " triUse " << triUse[tri1]);

        svtkDebugMacro("candidate tri2 " << tri2 << " [" << swapPts[0] << " " << swapPts[1] << " "
                                        << swapPts[2] << "]"
                                        << "triUse " << triUse[tri2]);

        // compute the normal for the 2 candidate triangles
        svtkTriangle::ComputeNormal(points, 3, pts, n1);
        svtkTriangle::ComputeNormal(points, 3, swapPts, n2);

        // the normals must be along the same direction,
        // or one triangle is upside down.
        if (svtkMath::Dot(n1, n2) < 0.0)
        {
          // do not swap diagonal
          continue;
        }

        // swap edge [p1 p2] and diagonal [ptId p3]
        this->Mesh->RemoveReferenceToCell(p1, tri2);
        this->Mesh->RemoveReferenceToCell(p2, tri1);
        this->Mesh->ResizeCellList(ptId, 1);
        this->Mesh->ResizeCellList(p3, 1);
        this->Mesh->AddReferenceToCell(ptId, tri2);
        this->Mesh->AddReferenceToCell(p3, tri1);

        // it's ok to swap the diagonal
        this->Mesh->ReplaceCell(tri1, 3, pts);
        this->Mesh->ReplaceCell(tri2, 3, swapPts);

        triUse[tri1] = (p1 < numPoints && p3 < numPoints);
        triUse[tri2] = (p3 < numPoints && p2 < numPoints);

        svtkDebugMacro("replace tri1 " << tri1 << " [" << pts[0] << " " << pts[1] << " " << pts[2]
                                      << "]"
                                      << " triUse " << triUse[tri1]);

        svtkDebugMacro("replace tri2 " << tri2 << " [" << swapPts[0] << " " << swapPts[1] << " "
                                      << swapPts[2] << "]"
                                      << " triUse " << triUse[tri2]);

        // update the 'scheduled for removal' flag of the first triangle.
        // The second triangle was not scheduled for removal anyway.
        numSwaps++;
        svtkDebugMacro("numSwaps " << numSwaps);
      }
    }
    svtkDebugMacro("numSwaps " << numSwaps);
  }

  // Update output; free up supporting data structures.
  //
  if (this->BoundingTriangulation && !this->Transform)
  {
    output->SetPoints(points);
  }
  else
  {
    output->SetPoints(inPoints);
    output->GetPointData()->PassData(input->GetPointData());
  }

  if (this->Alpha <= 0.0 && this->BoundingTriangulation && !source)
  {
    output->SetPolys(triangles);
  }
  else
  {
    svtkCellArray* alphaTriangles = svtkCellArray::New();
    alphaTriangles->AllocateEstimate(numTriangles, 3);
    const svtkIdType* alphaTriPts;

    for (i = 0; i < numTriangles; i++)
    {
      if (triUse[i])
      {
        this->Mesh->GetCellPoints(i, npts, alphaTriPts);
        alphaTriangles->InsertNextCell(3, alphaTriPts);
      }
    }
    output->SetPolys(alphaTriangles);
    alphaTriangles->Delete();
    delete[] triUse;
  }

  points->Delete();
  triangles->Delete();
  this->Mesh->Delete();
  neighbors->Delete();
  cells->Delete();

  // If the best fitting option was ON, then the current transform
  // is the one that was computed internally. We must now destroy it.
  if (this->ProjectionPlaneMode == SVTK_BEST_FITTING_PLANE)
  {
    if (this->Transform)
    {
      this->Transform->UnRegister(this);
      this->Transform->Delete();
      this->Transform = nullptr;
    }
  }

  output->Squeeze();

  return 1;
}

// Methods used to recover edges. Uses lines and polygons to determine boundary
// and inside/outside.
//
// Only the topology of the Source is used during the constrain operation.
// The point ids in the Source topology are assumed to reference points
// in the input. So, when an input transform is used, only the input points
// are transformed.  We do not bother with transforming the Source points
// since they are never referenced.
int* svtkDelaunay2D::RecoverBoundary(svtkPolyData* source)
{
  svtkCellArray* lines = source->GetLines();
  svtkCellArray* polys = source->GetPolys();
  const svtkIdType* pts = nullptr;
  svtkIdType npts = 0;
  svtkIdType i, p1, p2;
  int* triUse;

  source->BuildLinks();

  // Recover the edges of the mesh
  for (lines->InitTraversal(); lines->GetNextCell(npts, pts);)
  {
    for (i = 0; i < (npts - 1); i++)
    {
      p1 = pts[i];
      p2 = pts[i + 1];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        this->RecoverEdge(source, p1, p2);
      }
    }
  }

  // Recover the enclosed regions (polygons) of the mesh
  for (polys->InitTraversal(); polys->GetNextCell(npts, pts);)
  {
    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        this->RecoverEdge(source, p1, p2);
      }
    }
  }

  // Generate inside/outside marks on mesh
  int numTriangles = this->Mesh->GetNumberOfCells();
  triUse = new int[numTriangles];
  for (i = 0; i < numTriangles; i++)
  {
    triUse[i] = 1;
  }

  // Use any polygons to mark inside and outside. (Note that if an edge was not
  // recovered, we're going to have a problem.) The first polygon is assumed to
  // define the outside of the polygon; additional polygons carve out inside
  // holes.
  this->FillPolygons(polys, triUse);

  return triUse;
}

// Method attempts to recover an edge by retriangulating mesh around the edge.
// What we do is identify a "submesh" of triangles that includes the edge to recover.
// Then we split the submesh in two with the recovered edge, and triangulate each of
// the two halves. If any part of this fails, we leave things alone.
int svtkDelaunay2D::RecoverEdge(svtkPolyData* source, svtkIdType p1, svtkIdType p2)
{
  svtkIdType cellId = 0;
  int i, j, k;
  double p1X[3], p2X[3], xyNormal[3], splitNormal[3], p21[3];
  double x1[3], x2[3], sepNormal[3], v21[3];
  int ncells, v1 = 0, v2 = 0, signX1 = 0, signX2, signP1, signP2;
  const svtkIdType* pts;
  svtkIdType *leftTris, *rightTris, npts, numRightTris, numLeftTris;
  int success = 0, nbPts;

  svtkIdList* cells = svtkIdList::New();
  cells->Allocate(64);
  svtkIdList* tris = svtkIdList::New();
  tris->Allocate(64);
  svtkPolygon* rightPoly = svtkPolygon::New();
  svtkPolygon* leftPoly = svtkPolygon::New();
  svtkIdList* leftChain = leftPoly->GetPointIds();
  svtkIdList* rightChain = rightPoly->GetPointIds();
  svtkPoints* leftChainX = leftPoly->GetPoints();
  svtkPoints* rightChainX = rightPoly->GetPoints();
  svtkIdList* neis = svtkIdList::New();
  neis->Allocate(4);
  svtkIdList* rightPtIds = svtkIdList::New();
  rightPtIds->Allocate(64);
  svtkIdList* leftPtIds = svtkIdList::New();
  leftPtIds->Allocate(64);
  svtkPoints* rightTriPts = svtkPoints::New();
  rightTriPts->Allocate(64);
  svtkPoints* leftTriPts = svtkPoints::New();
  leftTriPts->Allocate(64);

  // Container for the edges (2 ids in a set, the order does not matter) we won't check
  std::set<std::set<svtkIdType> > polysEdges;
  // Container for the cells & point ids for the edge that need to be checked
  std::vector<svtkIdType> newEdges;

  // Compute a split plane along (p1,p2) and parallel to the z-axis.
  //
  this->GetPoint(p1, p1X);
  p1X[2] = 0.0; // split plane point
  this->GetPoint(p2, p2X);
  p2X[2] = 0.0; // split plane point
  xyNormal[0] = xyNormal[1] = 0.0;
  xyNormal[2] = 1.0;
  for (i = 0; i < 3; i++)
  {
    p21[i] = p2X[i] - p1X[i]; // working in x-y plane
  }

  svtkMath::Cross(p21, xyNormal, splitNormal);
  if (svtkMath::Normalize(splitNormal) == 0.0)
  { // Usually means coincident points
    goto FAILURE;
  }

  // Identify a triangle connected to the point p1 containing a portion of the edge.
  //
  this->Mesh->GetPointCells(p1, cells);
  ncells = cells->GetNumberOfIds();
  for (i = 0; i < ncells; i++)
  {
    cellId = cells->GetId(i);
    this->Mesh->GetCellPoints(cellId, npts, pts);
    for (j = 0; j < 3; j++)
    {
      if (pts[j] == p1)
      {
        break;
      }
    }
    v1 = pts[(j + 1) % 3];
    v2 = pts[(j + 2) % 3];
    this->GetPoint(v1, x1);
    x1[2] = 0.0;
    this->GetPoint(v2, x2);
    x2[2] = 0.0;
    signX1 = (svtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0 ? 1 : -1);
    signX2 = (svtkPlane::Evaluate(splitNormal, p1X, x2) > 0.0 ? 1 : -1);
    if (signX1 != signX2) // points of triangle on either side of edge
    {
      // determine if edge separates p1 from p2 - then we've found triangle
      v21[0] = x2[0] - x1[0]; // working in x-y plane
      v21[1] = x2[1] - x1[1];
      v21[2] = 0.0;

      svtkMath::Cross(v21, xyNormal, sepNormal);
      if (svtkMath::Normalize(sepNormal) == 0.0)
      { // bad mesh
        goto FAILURE;
      }

      signP1 = (svtkPlane::Evaluate(sepNormal, x1, p1X) > 0.0 ? 1 : -1);
      signP2 = (svtkPlane::Evaluate(sepNormal, x1, p2X) > 0.0 ? 1 : -1);
      if (signP1 != signP2) // is a separation line
      {
        break;
      }
    }
  } // for all cells

  if (i >= ncells)
  { // something is really screwed up
    goto FAILURE;
  }

  // We found initial triangle; begin to track triangles containing edge. Also,
  // the triangle defines the beginning of two "chains" which form a boundary of
  // enclosing triangles around the edge. Create the two chains (from p1 to p2).
  // (The chains are actually defining two polygons on either side of the edge.)
  //
  tris->InsertId(0, cellId);
  rightChain->InsertId(0, p1);
  rightChainX->InsertPoint(0, p1X);
  leftChain->InsertId(0, p1);
  leftChainX->InsertPoint(0, p1X);
  if (signX1 > 0)
  {
    rightChain->InsertId(1, v1);
    rightChainX->InsertPoint(1, x1);
    leftChain->InsertId(1, v2);
    leftChainX->InsertPoint(1, x2);
  }
  else
  {
    leftChain->InsertId(1, v1);
    leftChainX->InsertPoint(1, x1);
    rightChain->InsertId(1, v2);
    rightChainX->InsertPoint(1, x2);
  }

  // Walk along triangles (edge neighbors) towards point p2.
  while (v1 != p2)
  {
    this->Mesh->GetCellEdgeNeighbors(cellId, v1, v2, neis);
    if (neis->GetNumberOfIds() != 1)
    { // Mesh is folded or degenerate
      goto FAILURE;
    }
    cellId = neis->GetId(0);
    tris->InsertNextId(cellId);
    this->Mesh->GetCellPoints(cellId, npts, pts);
    for (j = 0; j < 3; j++)
    {
      if (pts[j] != v1 && pts[j] != v2)
      { // found point opposite current edge (v1,v2)
        if (pts[j] == p2)
        {
          v1 = p2; // this will cause the walk to stop
          rightChain->InsertNextId(p2);
          rightChainX->InsertNextPoint(p2X);
          leftChain->InsertNextId(p2);
          leftChainX->InsertNextPoint(p2X);
        }
        else
        { // keep walking
          this->GetPoint(pts[j], x1);
          x1[2] = 0.0;
          if (svtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0)
          {
            v1 = pts[j];
            rightChain->InsertNextId(v1);
            rightChainX->InsertNextPoint(x1);
          }
          else
          {
            v2 = pts[j];
            leftChain->InsertNextId(v2);
            leftChainX->InsertNextPoint(x1);
          }
        }
        break;
      } // else found opposite point
    }   // for all points in triangle
  }     // while walking

  // Fetch the left & right polygons edges
  nbPts = rightPoly->GetPointIds()->GetNumberOfIds();
  for (i = 0; i < nbPts; i++)
  {
    std::set<svtkIdType> edge;
    edge.insert(rightPoly->GetPointId(i));
    edge.insert(rightPoly->GetPointId((i + 1) % nbPts));
    polysEdges.insert(edge);
  }
  nbPts = leftPoly->GetPointIds()->GetNumberOfIds();
  for (i = 0; i < nbPts; i++)
  {
    std::set<svtkIdType> edge;
    edge.insert(leftPoly->GetPointId(i));
    edge.insert(leftPoly->GetPointId((i + 1) % nbPts));
    polysEdges.insert(edge);
  }

  // Now that the to chains are formed, each chain forms a polygon (along with
  // the edge (p1,p2)) that requires triangulation. If we can successfully
  // triangulate the two polygons, we will delete the triangles contained within
  // the chains and replace them with the new triangulation.
  //
  success = 1;
  success &= (rightPoly->BoundedTriangulate(rightPtIds, this->Tolerance));
  {
    svtkIdList* ids = svtkIdList::New();
    ids->Allocate(64);
    for (i = 0; i < rightPtIds->GetNumberOfIds(); i++)
    {
      ids->InsertId(i, rightPoly->PointIds->GetId(rightPtIds->GetId(i)));
    }
    rightPtIds->Delete();
    rightPtIds = ids;
  }
  numRightTris = rightPtIds->GetNumberOfIds() / 3;

  success &= (leftPoly->BoundedTriangulate(leftPtIds, this->Tolerance));
  {
    svtkIdList* ids = svtkIdList::New();
    ids->Allocate(64);
    for (i = 0; i < leftPtIds->GetNumberOfIds(); i++)
    {
      ids->InsertId(i, leftPoly->PointIds->GetId(leftPtIds->GetId(i)));
    }
    leftPtIds->Delete();
    leftPtIds = ids;
  }
  numLeftTris = leftPtIds->GetNumberOfIds() / 3;

  if (!success)
  { // polygons on either side of edge are poorly shaped
    goto FAILURE;
  }

  // Okay, delete the old triangles and replace them with new ones. There should be
  // the same number of new triangles as old ones.
  leftTris = leftPtIds->GetPointer(0);
  for (j = i = 0; i < numLeftTris; i++, j++, leftTris += 3)
  {
    cellId = tris->GetId(j);
    this->Mesh->RemoveCellReference(cellId);
    for (k = 0; k < 3; k++)
    { // allocate new space for cell lists
      this->Mesh->ResizeCellList(leftTris[k], 1);
    }
    this->Mesh->ReplaceLinkedCell(cellId, 3, leftTris);

    // Check if the added triangle contains edges which are not in the polygon edges set
    for (int e = 0; e < 3; e++)
    {
      svtkIdType ep1 = leftTris[e];
      svtkIdType ep2 = leftTris[(e + 1) % 3];
      svtkIdType ep3 = leftTris[(e + 2) % 3];
      // Make sure we won't alter a constrained edge
      if (!source->IsEdge(ep1, ep2) && !source->IsEdge(ep2, ep3) && !source->IsEdge(ep3, ep1))
      {
        std::set<svtkIdType> edge;
        edge.insert(ep1);
        edge.insert(ep2);
        if (polysEdges.find(edge) == polysEdges.end())
        {
          // Add this new edge and remember current triangle and third point ids too
          newEdges.push_back(cellId);
          newEdges.push_back(ep1);
          newEdges.push_back(ep2);
          newEdges.push_back(ep3);
        }
      }
    }
  }

  rightTris = rightPtIds->GetPointer(0);
  for (i = 0; i < numRightTris; i++, j++, rightTris += 3)
  {
    cellId = tris->GetId(j);
    this->Mesh->RemoveCellReference(cellId);
    for (k = 0; k < 3; k++)
    { // allocate new space for cell lists
      this->Mesh->ResizeCellList(rightTris[k], 1);
    }
    this->Mesh->ReplaceLinkedCell(cellId, 3, rightTris);

    // Check if the added triangle contains edges which are not in the polygon edges set
    for (int e = 0; e < 3; e++)
    {
      svtkIdType ep1 = rightTris[e];
      svtkIdType ep2 = rightTris[(e + 1) % 3];
      svtkIdType ep3 = rightTris[(e + 2) % 3];
      // Make sure we won't alter a constrained edge
      if (!source->IsEdge(ep1, ep2) && !source->IsEdge(ep2, ep3) && !source->IsEdge(ep3, ep1))
      {
        std::set<svtkIdType> edge;
        edge.insert(ep1);
        edge.insert(ep2);
        if (polysEdges.find(edge) == polysEdges.end())
        {
          // Add this new edge and remember current triangle and third point ids too
          newEdges.push_back(cellId);
          newEdges.push_back(ep1);
          newEdges.push_back(ep2);
          newEdges.push_back(ep3);
        }
      }
    }
  }

  j = static_cast<int>(newEdges.size()) / 4;
  // Now check the new suspicious edges
  for (i = 0; i < j; i++)
  {
    svtkIdType* v = &newEdges[4 * i];
    double x[3];
    this->GetPoint(v[3], x);
    this->CheckEdge(v[3], x, v[1], v[2], v[0], false);
  }

FAILURE:
  tris->Delete();
  cells->Delete();
  leftPoly->Delete();
  rightPoly->Delete();
  neis->Delete();
  rightPtIds->Delete();
  leftPtIds->Delete();
  rightTriPts->Delete();
  leftTriPts->Delete();
  return success;
}

void svtkDelaunay2D::FillPolygons(svtkCellArray* polys, int* triUse)
{
  svtkIdType p1, p2, j, kk;
  int i, k;
  const svtkIdType* pts = nullptr;
  const svtkIdType* triPts;
  svtkIdType npts = 0;
  svtkIdType numPts;
  static double xyNormal[3] = { 0.0, 0.0, 1.0 };
  double negDir[3], x21[3], x1[3], x2[3], x[3];
  svtkIdList* neis = svtkIdList::New();
  svtkIdType cellId, numNeis;
  svtkIdList *currentFront = svtkIdList::New(), *tmpFront;
  svtkIdList* nextFront = svtkIdList::New();
  svtkIdType numCellsInFront, neiId;
  svtkIdType numTriangles = this->Mesh->GetNumberOfCells();

  // Loop over edges of polygon, marking triangles on "outside" of polygon as outside.
  // Then perform a fill.
  for (polys->InitTraversal(); polys->GetNextCell(npts, pts);)
  {
    currentFront->Reset();
    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        svtkWarningMacro(<< "Edge not recovered, polygon fill suspect");
      }
      else // Mark the "outside" triangles
      {
        neis->Reset();
        this->GetPoint(p1, x1);
        this->GetPoint(p2, x2);
        for (j = 0; j < 3; j++)
        {
          x21[j] = x2[j] - x1[j];
        }
        svtkMath::Cross(x21, xyNormal, negDir);
        this->Mesh->GetCellEdgeNeighbors(-1, p1, p2, neis); // get both triangles
        numNeis = neis->GetNumberOfIds();
        for (j = 0; j < numNeis; j++)
        { // find the vertex not on the edge; evaluate it (and the cell) in/out
          cellId = neis->GetId(j);
          this->Mesh->GetCellPoints(cellId, numPts, triPts);
          for (k = 0; k < 3; k++)
          {
            if (triPts[k] != p1 && triPts[k] != p2)
            {
              break;
            }
          }
          this->GetPoint(triPts[k], x);
          x[2] = 0.0;
          if (svtkPlane::Evaluate(negDir, x1, x) > 0.0)
          {
            triUse[cellId] = 0;
            currentFront->InsertNextId(cellId);
          }
          else
          {
            triUse[cellId] = -1;
          }
        }
      } // edge was recovered
    }   // for all edges in polygon

    // Okay, now perform a fill operation (filling "outside" values).
    //
    while ((numCellsInFront = currentFront->GetNumberOfIds()) > 0)
    {
      for (j = 0; j < numCellsInFront; j++)
      {
        cellId = currentFront->GetId(j);

        this->Mesh->GetCellPoints(cellId, numPts, triPts);
        for (k = 0; k < 3; k++)
        {
          p1 = triPts[k];
          p2 = triPts[(k + 1) % 3];

          this->Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neis);
          numNeis = neis->GetNumberOfIds();
          for (kk = 0; kk < numNeis; kk++)
          {
            neiId = neis->GetId(kk);
            if (triUse[neiId] == 1) // 0 is what we're filling with
            {
              triUse[neiId] = 0;
              nextFront->InsertNextId(neiId);
            }
          } // mark all neighbors
        }   // for all edges of cell
      }     // all cells in front

      tmpFront = currentFront;
      currentFront = nextFront;
      nextFront = tmpFront;
      nextFront->Reset();
    } // while still advancing

  } // for all polygons

  // convert all unvisited to inside
  for (i = 0; i < numTriangles; i++)
  {
    if (triUse[i] == -1)
    {
      triUse[i] = 1;
    }
  }

  currentFront->Delete();
  nextFront->Delete();
  neis->Delete();
}

//----------------------------------------------------------------------------
int svtkDelaunay2D::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
svtkAbstractTransform* svtkDelaunay2D::ComputeBestFittingPlane(svtkPointSet* input)
{
  svtkIdType numPts = input->GetNumberOfPoints();
  double m[9], v[3], x[3];
  svtkIdType ptId;
  int i;
  double *c1, *c2, *c3, det;
  double normal[3];
  double origin[3];

  const double tolerance = 1.0e-03;

  //  This code was taken from the svtkTextureMapToPlane class
  //  and slightly modified.
  //
  for (i = 0; i < 3; i++)
  {
    normal[i] = 0.0;
  }

  //  Get minimum width of bounding box.
  const double* bounds = input->GetBounds();
  double length = input->GetLength();
  int dir = 0;
  double w;

  for (w = length, i = 0; i < 3; i++)
  {
    normal[i] = 0.0;
    if ((bounds[2 * i + 1] - bounds[2 * i]) < w)
    {
      dir = i;
      w = bounds[2 * i + 1] - bounds[2 * i];
    }
  }

  //  If the bounds is perpendicular to one of the axes, then can
  //  quickly compute normal.
  //
  normal[dir] = 1.0;
  bool normal_computed = false;
  if (w <= (length * tolerance))
  {
    normal_computed = true;
  }

  //  Compute least squares approximation.
  //  Compute 3x3 least squares matrix
  v[0] = v[1] = v[2] = 0.0;
  for (i = 0; i < 9; i++)
  {
    m[i] = 0.0;
  }

  for (ptId = 0; ptId < numPts; ptId++)
  {
    input->GetPoint(ptId, x);

    v[0] += x[0] * x[2];
    v[1] += x[1] * x[2];
    v[2] += x[2];

    m[0] += x[0] * x[0];
    m[1] += x[0] * x[1];
    m[2] += x[0];

    m[3] += x[0] * x[1];
    m[4] += x[1] * x[1];
    m[5] += x[1];

    m[6] += x[0];
    m[7] += x[1];
  }
  m[8] = numPts;

  origin[0] = m[2] / numPts;
  origin[1] = m[5] / numPts;
  origin[2] = v[2] / numPts;

  //  Solve linear system using Kramers rule
  //
  c1 = m;
  c2 = m + 3;
  c3 = m + 6;
  if (!normal_computed && (det = svtkMath::Determinant3x3(c1, c2, c3)) > tolerance)
  {
    normal[0] = svtkMath::Determinant3x3(v, c2, c3) / det;
    normal[1] = svtkMath::Determinant3x3(c1, v, c3) / det;
    normal[2] = -1.0; // because of the formulation
  }

  svtkTransform* transform = svtkTransform::New();

  // Set the new Z axis as the normal to the best fitting
  // plane.
  double zaxis[3];
  zaxis[0] = 0;
  zaxis[1] = 0;
  zaxis[2] = 1;

  double rotationAxis[3];

  svtkMath::Normalize(normal);
  svtkMath::Cross(normal, zaxis, rotationAxis);
  svtkMath::Normalize(rotationAxis);

  const double rotationAngle = 180.0 * acos(svtkMath::Dot(zaxis, normal)) / svtkMath::Pi();

  transform->PreMultiply();
  transform->Identity();

  transform->RotateWXYZ(rotationAngle, rotationAxis[0], rotationAxis[1], rotationAxis[2]);

  // Set the center of mass as the origin of coordinates
  transform->Translate(-origin[0], -origin[1], -origin[2]);

  return transform;
}

//----------------------------------------------------------------------------
void svtkDelaunay2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "ProjectionPlaneMode: "
     << ((this->ProjectionPlaneMode == SVTK_BEST_FITTING_PLANE) ? "Best Fitting Plane" : "XY Plane")
     << "\n";
  os << indent << "Transform: " << (this->Transform ? "specified" : "none") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: " << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
