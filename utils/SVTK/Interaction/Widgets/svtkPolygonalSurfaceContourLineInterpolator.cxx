/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolygonalSurfaceContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolygonalSurfaceContourLineInterpolator.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkContourRepresentation.h"
#include "svtkDijkstraGraphGeodesicPath.h"
#include "svtkIdList.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygonalSurfacePointPlacer.h"

svtkStandardNewMacro(svtkPolygonalSurfaceContourLineInterpolator);

//----------------------------------------------------------------------
svtkPolygonalSurfaceContourLineInterpolator ::svtkPolygonalSurfaceContourLineInterpolator()
{
  this->LastInterpolatedVertexIds[0] = -1;
  this->LastInterpolatedVertexIds[1] = -1;
  this->DistanceOffset = 0.0;
  this->DijkstraGraphGeodesicPath = svtkDijkstraGraphGeodesicPath::New();
}

//----------------------------------------------------------------------
svtkPolygonalSurfaceContourLineInterpolator ::~svtkPolygonalSurfaceContourLineInterpolator()
{
  this->DijkstraGraphGeodesicPath->Delete();
}

//----------------------------------------------------------------------
int svtkPolygonalSurfaceContourLineInterpolator::UpdateNode(
  svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node), int svtkNotUsed(idx))
{
  return 0;
}

//----------------------------------------------------------------------
int svtkPolygonalSurfaceContourLineInterpolator::InterpolateLine(
  svtkRenderer*, svtkContourRepresentation* rep, int idx1, int idx2)
{
  svtkPolygonalSurfacePointPlacer* placer =
    svtkPolygonalSurfacePointPlacer::SafeDownCast(rep->GetPointPlacer());
  if (!placer)
  {
    return 1;
  }

  double p1[3], p2[3], p[3];
  rep->GetNthNodeWorldPosition(idx1, p1);
  rep->GetNthNodeWorldPosition(idx2, p2);

  typedef svtkPolygonalSurfacePointPlacer::Node NodeType;
  NodeType* nodeBegin = placer->GetNodeAtWorldPosition(p1);
  NodeType* nodeEnd = placer->GetNodeAtWorldPosition(p2);
  if (nodeBegin->PolyData != nodeEnd->PolyData)
  {
    return 1;
  }

  // Find the starting and ending point id's
  svtkIdType beginVertId = -1, endVertId = -1;
  double minDistance;
  if (nodeBegin->CellId == -1)
  {
    // If no cell is specified, use the pointid instead
    beginVertId = nodeBegin->PointId;
  }
  else
  {
    svtkCell* cellBegin = nodeBegin->PolyData->GetCell(nodeBegin->CellId);
    svtkPoints* cellBeginPoints = cellBegin->GetPoints();

    minDistance = SVTK_DOUBLE_MAX;
    for (int i = 0; i < cellBegin->GetNumberOfPoints(); i++)
    {
      cellBeginPoints->GetPoint(i, p);
      double distance = svtkMath::Distance2BetweenPoints(p, p1);
      if (distance < minDistance)
      {
        beginVertId = cellBegin->GetPointId(i);
        minDistance = distance;
      }
    }
  }

  if (nodeEnd->CellId == -1)
  {
    // If no cell is specified, use the pointid instead
    endVertId = nodeEnd->PointId;
  }
  else
  {
    svtkCell* cellEnd = nodeEnd->PolyData->GetCell(nodeEnd->CellId);
    svtkPoints* cellEndPoints = cellEnd->GetPoints();

    minDistance = SVTK_DOUBLE_MAX;
    for (int i = 0; i < cellEnd->GetNumberOfPoints(); i++)
    {
      cellEndPoints->GetPoint(i, p);
      double distance = svtkMath::Distance2BetweenPoints(p, p2);
      if (distance < minDistance)
      {
        endVertId = cellEnd->GetPointId(i);
        minDistance = distance;
      }
    }
  }

  if (beginVertId == -1 || endVertId == -1)
  {
    // Could not find the starting and ending cells. We can't interpolate.
    return 0;
  }

  // Now compute the shortest path through the surface mesh along its
  // edges using Dijkstra.

  this->DijkstraGraphGeodesicPath->SetInputData(nodeBegin->PolyData);
  this->DijkstraGraphGeodesicPath->SetStartVertex(endVertId);
  this->DijkstraGraphGeodesicPath->SetEndVertex(beginVertId);
  this->DijkstraGraphGeodesicPath->Update();

  svtkPolyData* pd = this->DijkstraGraphGeodesicPath->GetOutput();

  // We assume there's only one cell of course
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  pd->GetLines()->InitTraversal();
  pd->GetLines()->GetNextCell(npts, pts);

  // Get the vertex normals if there is a height offset. The offset at
  // each node of the graph is in the direction of the vertex normal.

  svtkIdList* vertexIds = this->DijkstraGraphGeodesicPath->GetIdList();
  double vertexNormal[3];
  svtkDataArray* vertexNormals = nullptr;
  if (this->DistanceOffset != 0.0)
  {
    vertexNormals = nodeBegin->PolyData->GetPointData()->GetNormals();
  }

  for (int n = 0; n < npts; n++)
  {
    pd->GetPoint(pts[n], p);

    // This is the id of the point on the polygonal surface.
    const svtkIdType ptId = vertexIds->GetId(n);

    // Offset the point in the direction of the normal, if a distance
    // offset is specified.
    if (vertexNormals)
    {
      vertexNormals->GetTuple(ptId, vertexNormal);
      p[0] += vertexNormal[0] * this->DistanceOffset;
      p[1] += vertexNormal[1] * this->DistanceOffset;
      p[2] += vertexNormal[2] * this->DistanceOffset;
    }

    // Add this point as an intermediate node of the contour. Store tehe
    // ptId if necessary.
    rep->AddIntermediatePointWorldPosition(idx1, p, ptId);
  }

  this->LastInterpolatedVertexIds[0] = beginVertId;
  this->LastInterpolatedVertexIds[1] = endVertId;

  // Also set the start and end node on the contour rep
  rep->GetNthNode(idx1)->PointId = beginVertId;
  rep->GetNthNode(idx2)->PointId = endVertId;

  return 1;
}

//----------------------------------------------------------------------
void svtkPolygonalSurfaceContourLineInterpolator ::GetContourPointIds(
  svtkContourRepresentation* rep, svtkIdList* ids)
{
  // Get the number of points in the contour and pre-allocate size

  const int nNodes = rep->GetNumberOfNodes();

  svtkIdType nPoints = 0;
  for (int i = 0; i < nNodes; i++)
  {
    // 1 for the node and then the number of points.
    nPoints += static_cast<svtkIdType>(rep->GetNthNode(i)->Points.size() + 1);
  }

  ids->SetNumberOfIds(nPoints);

  // Now fill the point ids

  int idx = 0;
  for (int i = 0; i < nNodes; i++)
  {
    svtkContourRepresentationNode* node = rep->GetNthNode(i);
    ids->SetId(idx++, node->PointId);
    const int nIntermediatePts = static_cast<int>(node->Points.size());

    for (int j = 0; j < nIntermediatePts; j++)
    {
      ids->SetId(idx++, node->Points[j]->PointId);
    }
  }
}

//----------------------------------------------------------------------
void svtkPolygonalSurfaceContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DistanceOffset: " << this->DistanceOffset << endl;
}
