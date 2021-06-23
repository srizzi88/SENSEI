/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClosedSurfacePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClosedSurfacePointPlacer.h"
#include "svtkCamera.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPlanes.h"
#include "svtkRenderer.h"

#include <algorithm>
#include <vector>

svtkStandardNewMacro(svtkClosedSurfacePointPlacer);
svtkCxxSetObjectMacro(svtkClosedSurfacePointPlacer, BoundingPlanes, svtkPlaneCollection);

//----------------------------------------------------------------------
// Place holder structure to find the two planes that would best cut
// a line with a plane. We do this freaky stuff because we cannot use
// absolute tolerances. Sometimes a point may be intersected by two planes
// when it is on a corner etc... Believe me, I found this necessary.
//
// Plane   : The plane that we found had intersected the line in question
// p       : The intersection point of the line and the plane.
// Distance: Distance of the point "p" from the object. Negative distances
//           mean that it is outside.
struct svtkClosedSurfacePointPlacerNode
{
  typedef svtkClosedSurfacePointPlacerNode Self;
  mutable svtkPlane* Plane;
  double Distance;
  double p[3];
  static bool Sort(const Self& a, const Self& b) { return a.Distance > b.Distance; }
  bool operator==(const Self& a) const { return a.Plane == this->Plane; }
  bool operator!=(const Self& a) const { return a.Plane != this->Plane; }
  svtkClosedSurfacePointPlacerNode()
  {
    Plane = nullptr;
    Distance = SVTK_DOUBLE_MIN;
  }
};

//----------------------------------------------------------------------
svtkClosedSurfacePointPlacer::svtkClosedSurfacePointPlacer()
{
  this->BoundingPlanes = nullptr;
  this->MinimumDistance = 0.0;
  this->InnerBoundingPlanes = svtkPlaneCollection::New();
}

//----------------------------------------------------------------------
svtkClosedSurfacePointPlacer::~svtkClosedSurfacePointPlacer()
{
  this->RemoveAllBoundingPlanes();
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->UnRegister(this);
  }
  this->InnerBoundingPlanes->Delete();
}

//----------------------------------------------------------------------
void svtkClosedSurfacePointPlacer::AddBoundingPlane(svtkPlane* plane)
{
  if (this->BoundingPlanes == nullptr)
  {
    this->BoundingPlanes = svtkPlaneCollection::New();
    this->BoundingPlanes->Register(this);
    this->BoundingPlanes->Delete();
  }

  this->BoundingPlanes->AddItem(plane);
}

//----------------------------------------------------------------------
void svtkClosedSurfacePointPlacer::RemoveBoundingPlane(svtkPlane* plane)
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveItem(plane);
  }
}

//----------------------------------------------------------------------
void svtkClosedSurfacePointPlacer::RemoveAllBoundingPlanes()
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = nullptr;
  }
}
//----------------------------------------------------------------------

void svtkClosedSurfacePointPlacer::SetBoundingPlanes(svtkPlanes* planes)
{
  if (!planes)
  {
    return;
  }

  svtkPlane* plane;
  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllBoundingPlanes();
  for (int i = 0; i < numPlanes; i++)
  {
    plane = svtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddBoundingPlane(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
void svtkClosedSurfacePointPlacer::BuildPlanes()
{
  if (this->InnerBoundingPlanes->GetMTime() > this->GetMTime() &&
    this->InnerBoundingPlanes->GetMTime() > this->BoundingPlanes->GetMTime())
  {
    return;
  }

  // Need to build planes.. Bring them all in front by MinimumDistance.
  // Find the Inner bounding planes.

  this->InnerBoundingPlanes->RemoveAllItems();

  double normal[3], origin[3];
  svtkPlane* p;
  for (this->BoundingPlanes->InitTraversal(); (p = this->BoundingPlanes->GetNextItem());)
  {
    p->GetNormal(normal);
    p->GetOrigin(origin);
    for (int i = 0; i < 3; i++)
    {
      origin[i] += this->MinimumDistance * normal[i];
    }

    svtkPlane* plane = svtkPlane::New();
    plane->SetOrigin(origin);
    plane->SetNormal(normal);
    this->InnerBoundingPlanes->AddItem(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
// Given a renderer, a display position and a reference position, "worldPos"
// is calculated as :
//   Consider the line "L" that passes through the supplied "displayPos" and
// is parallel to the direction of projection of the camera. Clip this line
// segment with the parallelopiped, let's call it "L_segment". The computed
// world position, "worldPos" will be the point on "L_segment" that is closest
// to refWorldPos.
int svtkClosedSurfacePointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double refWorldPos[3], double worldPos[3], double* svtkNotUsed(worldOrient))
{
  this->BuildPlanes();

  if (!this->BoundingPlanes)
  {
    return 0;
  }

  double directionOfProjection[3], t, d[3], currentWorldPos[4], ls[2][3], fp[4];

  svtkInteractorObserver::ComputeWorldToDisplay(
    ren, refWorldPos[0], refWorldPos[1], refWorldPos[2], fp);

  ren->GetActiveCamera()->GetDirectionOfProjection(directionOfProjection);
  svtkInteractorObserver::ComputeDisplayToWorld(
    ren, displayPos[0], displayPos[1], fp[2], currentWorldPos);

  // The line "L" defined by two points, l0 and l1. The line-segment
  // end-points will be defined by points ls[2][3].
  double l0[3] = { currentWorldPos[0] - directionOfProjection[0],
    currentWorldPos[1] - directionOfProjection[1], currentWorldPos[2] - directionOfProjection[2] };
  double l1[3] = { currentWorldPos[0] + directionOfProjection[0],
    currentWorldPos[1] + directionOfProjection[1], currentWorldPos[2] + directionOfProjection[2] };

  // Traverse all the planes to clip the line.

  svtkPlaneCollection* pc = this->InnerBoundingPlanes;

  // Stores candidate intersections with the parallelopiped. This was found
  // necessary instead of a simple two point intersection test because of
  // tolerances in svtkPlane::EvaluatePosition when the handle was very close
  // to an edge.
  std::vector<svtkClosedSurfacePointPlacerNode> intersections;

  const int nPlanes = pc->GetNumberOfItems();

  // Loop over each plane.
  for (int n = 0; n < nPlanes; n++)
  {
    svtkPlane* plane = static_cast<svtkPlane*>(pc->GetItemAsObject(n));
    svtkClosedSurfacePointPlacerNode node;

    svtkPlane::IntersectWithLine(l0, l1, plane->GetNormal(), plane->GetOrigin(), t, node.p);

    // The IF below insures that the line and the plane aren't parallel.
    if (t != SVTK_DOUBLE_MAX)
    {
      node.Plane = plane;
      node.Distance = this->GetDistanceFromObject(node.p, this->InnerBoundingPlanes, d);
      intersections.push_back(node);
      svtkDebugMacro(<< "We aren't parallel to plane with normal: (" << plane->GetNormal()[0] << ","
                    << plane->GetNormal()[1] << "," << plane->GetNormal()[2] << ")");
      svtkDebugMacro(<< "Size of inersections = " << intersections.size()
                    << " Distance: " << node.Distance << " Plane: " << plane);
    }
  }

  std::sort(intersections.begin(), intersections.end(), svtkClosedSurfacePointPlacerNode::Sort);

  // Now pick the top two candidates, insuring that the line at least intersects
  // with the object. If we have fewer than 2 in the queue, or if the
  // top candidate is outsude, we have failed to intersect the object.

  std::vector<svtkClosedSurfacePointPlacerNode>::const_iterator it = intersections.begin();
  if (intersections.size() < 2 || it->Distance < (-1.0 * this->WorldTolerance) ||
    (++it)->Distance < (-1.0 * this->WorldTolerance))
  {
    // The display point points to a location outside the object. Just
    // return 0. In actuality, I'd like to return the closest point in the
    // object. For this I require an algorithm that can, given a point "p" and
    // an object "O", defined by a set of bounding planes, find the point on
    // "O" that is closest to "p"

    return 0;
  }

  it = intersections.begin();
  for (int i = 0; i < 2; i++, ++it)
  {
    ls[i][0] = it->p[0];
    ls[i][1] = it->p[1];
    ls[i][2] = it->p[2];
  }

  svtkLine::DistanceToLine(refWorldPos, ls[0], ls[1], t, worldPos);
  t = (t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t));

  // the point "worldPos", now lies within the object and on the line from
  // the eye along the direction of projection.

  worldPos[0] = ls[0][0] * (1.0 - t) + ls[1][0] * t;
  worldPos[1] = ls[0][1] * (1.0 - t) + ls[1][1] * t;
  worldPos[2] = ls[0][2] * (1.0 - t) + ls[1][2] * t;

  svtkDebugMacro(<< "Reference Pos: (" << refWorldPos[0] << "," << refWorldPos[1] << ","
                << refWorldPos[2] << ")  Line segment from "
                << "the eye along the direction of projection, clipped by the object [(" << ls[0][0]
                << "," << ls[0][1] << "," << ls[0][2] << ") - (" << ls[1][0] << "," << ls[1][1]
                << "," << ls[1][2] << ")] Computed position (that is "
                << "the closest point on this segment to ReferencePos: (" << worldPos[0] << ","
                << worldPos[1] << "," << worldPos[2] << ")");

  return 1;
}

//----------------------------------------------------------------------
int svtkClosedSurfacePointPlacer ::ComputeWorldPosition(svtkRenderer*,
  double svtkNotUsed(displayPos)[2], double svtkNotUsed(worldPos)[3],
  double svtkNotUsed(worldOrient)[9])
{
  svtkErrorMacro(<< "This placer needs a reference world position.");
  return 0;
}

//----------------------------------------------------------------------
int svtkClosedSurfacePointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkClosedSurfacePointPlacer::ValidateWorldPosition(double worldPos[3])
{
  this->BuildPlanes();

  // Now check against the bounding planes
  if (this->InnerBoundingPlanes)
  {
    svtkPlane* p;

    this->InnerBoundingPlanes->InitTraversal();

    while ((p = this->InnerBoundingPlanes->GetNextItem()))
    {
      if (p->EvaluateFunction(worldPos) < this->WorldTolerance)
      {
        return 0;
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------
// Calculate the distance of a point from the Object. Negative
// values imply that the point is outside. Positive values imply that it is
// inside. The closest point to the object is returned in closestPt.
double svtkClosedSurfacePointPlacer ::GetDistanceFromObject(
  double pos[3], svtkPlaneCollection* pc, double closestPt[3])
{
  svtkPlane* minPlane = nullptr;
  double minD = SVTK_DOUBLE_MAX;

  pc->InitTraversal();
  while (svtkPlane* p = pc->GetNextItem())
  {
    const double d = p->EvaluateFunction(pos);
    if (d < minD)
    {
      minD = d;
      minPlane = p;
    }
  }

  svtkPlane::ProjectPoint(pos, minPlane->GetOrigin(), minPlane->GetNormal(), closestPt);
  return minD;
}

//----------------------------------------------------------------------
void svtkClosedSurfacePointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Bounding Planes:\n";
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << " (none)\n";
  }

  os << indent << "Minimum Distance: " << this->MinimumDistance << "\n";
}
