/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoundedPlanePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBoundedPlanePointPlacer.h"
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

svtkStandardNewMacro(svtkBoundedPlanePointPlacer);

svtkCxxSetObjectMacro(svtkBoundedPlanePointPlacer, ObliquePlane, svtkPlane);
svtkCxxSetObjectMacro(svtkBoundedPlanePointPlacer, BoundingPlanes, svtkPlaneCollection);

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
struct svtkBoundedPlanePointPlacerNode
{
  typedef svtkBoundedPlanePointPlacerNode Self;
  mutable svtkPlane* Plane;
  double Distance;
  double p[3];
  static bool Sort(const Self& a, const Self& b) { return a.Distance > b.Distance; }
  bool operator==(const Self& a) const { return a.Plane == this->Plane; }
  bool operator!=(const Self& a) const { return a.Plane != this->Plane; }
  svtkBoundedPlanePointPlacerNode()
  {
    Plane = nullptr;
    Distance = SVTK_DOUBLE_MIN;
  }
};

//----------------------------------------------------------------------
svtkBoundedPlanePointPlacer::svtkBoundedPlanePointPlacer()
{
  this->ProjectionPosition = 0;
  this->ObliquePlane = nullptr;
  this->ProjectionNormal = svtkBoundedPlanePointPlacer::ZAxis;
  this->BoundingPlanes = nullptr;
}

//----------------------------------------------------------------------
svtkBoundedPlanePointPlacer::~svtkBoundedPlanePointPlacer()
{
  this->RemoveAllBoundingPlanes();

  if (this->ObliquePlane)
  {
    this->ObliquePlane->UnRegister(this);
    this->ObliquePlane = nullptr;
  }

  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->UnRegister(this);
  }
}

//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::SetProjectionPosition(double position)
{
  if (this->ProjectionPosition != position)
  {
    this->ProjectionPosition = position;
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::AddBoundingPlane(svtkPlane* plane)
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
void svtkBoundedPlanePointPlacer::RemoveBoundingPlane(svtkPlane* plane)
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveItem(plane);
  }
}

//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::RemoveAllBoundingPlanes()
{
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = nullptr;
  }
}
//----------------------------------------------------------------------

void svtkBoundedPlanePointPlacer::SetBoundingPlanes(svtkPlanes* planes)
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
int svtkBoundedPlanePointPlacer::ComputeWorldPosition(svtkRenderer* ren, double displayPos[2],
  double svtkNotUsed(refWorldPos)[3], double worldPos[3], double worldOrient[9])
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int svtkBoundedPlanePointPlacer::ComputeWorldPosition(
  svtkRenderer* ren, double displayPos[2], double worldPos[3], double worldOrient[9])
{
  double nearWorldPoint[4];
  double farWorldPoint[4];
  double tmp[3];

  tmp[0] = displayPos[0];
  tmp[1] = displayPos[1];
  tmp[2] = 0.0; // near plane

  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(nearWorldPoint);

  tmp[2] = 1.0; // far plane
  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(farWorldPoint);

  double normal[3];
  double origin[3];

  this->GetProjectionNormal(normal);
  this->GetProjectionOrigin(origin);

  double position[3];
  double distance;
  if (svtkPlane::IntersectWithLine(
        nearWorldPoint, farWorldPoint, normal, origin, distance, position))
  {
    // Fill in the information now before validating it.
    // This is because we should return the best information
    // we can since this may be part of an UpdateWorldPosition
    // call - we need to do the best at updating the position
    // even if it is not valid.
    this->GetCurrentOrientation(worldOrient);
    worldPos[0] = position[0];
    worldPos[1] = position[1];
    worldPos[2] = position[2];

    // Now check against the bounding planes
    if (this->BoundingPlanes)
    {
      svtkPlane* p;

      this->BoundingPlanes->InitTraversal();

      while ((p = this->BoundingPlanes->GetNextItem()))
      {
        if (p->EvaluateFunction(position) < this->WorldTolerance)
        {
          return 0;
        }
      }
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------
int svtkBoundedPlanePointPlacer::ValidateWorldPosition(
  double worldPos[3], double* svtkNotUsed(worldOrient))
{
  return this->ValidateWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int svtkBoundedPlanePointPlacer::ValidateWorldPosition(double worldPos[3])
{
  // Now check against the bounding planes
  if (this->BoundingPlanes)
  {
    svtkPlane* p;

    this->BoundingPlanes->InitTraversal();

    while ((p = this->BoundingPlanes->GetNextItem()))
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
int svtkBoundedPlanePointPlacer::UpdateWorldPosition(
  svtkRenderer* ren, double worldPos[3], double worldOrient[9])
{
  double displayPoint[2];
  double tmp[4];

  tmp[0] = worldPos[0];
  tmp[1] = worldPos[1];
  tmp[2] = worldPos[2];
  tmp[3] = 1.0;

  ren->SetWorldPoint(tmp);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(tmp);

  displayPoint[0] = tmp[0];
  displayPoint[1] = tmp[1];

  return this->ComputeWorldPosition(ren, displayPoint, worldPos, worldOrient);
}
//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::GetCurrentOrientation(double worldOrient[9])
{
  double* x = worldOrient;
  double* y = worldOrient + 3;
  double* z = worldOrient + 6;

  this->GetProjectionNormal(z);

  double v[3];
  if (fabs(z[0]) >= fabs(z[1]) && fabs(z[0]) >= fabs(z[2]))
  {
    v[0] = 0;
    v[1] = 1;
    v[2] = 0;
  }
  else
  {
    v[0] = 1;
    v[1] = 0;
    v[2] = 0;
  }

  svtkMath::Cross(z, v, y);
  svtkMath::Cross(y, z, x);
}

//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::GetProjectionNormal(double normal[3])
{
  switch (this->ProjectionNormal)
  {
    case svtkBoundedPlanePointPlacer::XAxis:
      normal[0] = 1.0;
      normal[1] = 0.0;
      normal[2] = 0.0;
      break;
    case svtkBoundedPlanePointPlacer::YAxis:
      normal[0] = 0.0;
      normal[1] = 1.0;
      normal[2] = 0.0;
      break;
    case svtkBoundedPlanePointPlacer::ZAxis:
      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 1.0;
      break;
    case svtkBoundedPlanePointPlacer::Oblique:
      this->ObliquePlane->GetNormal(normal);
      break;
  }
}

//----------------------------------------------------------------------
void svtkBoundedPlanePointPlacer::GetProjectionOrigin(double origin[3])
{
  switch (this->ProjectionNormal)
  {
    case svtkBoundedPlanePointPlacer::XAxis:
      origin[0] = this->ProjectionPosition;
      origin[1] = 0.0;
      origin[2] = 0.0;
      break;
    case svtkBoundedPlanePointPlacer::YAxis:
      origin[0] = 0.0;
      origin[1] = this->ProjectionPosition;
      origin[2] = 0.0;
      break;
    case svtkBoundedPlanePointPlacer::ZAxis:
      origin[0] = 0.0;
      origin[1] = 0.0;
      origin[2] = this->ProjectionPosition;
      break;
    case svtkBoundedPlanePointPlacer::Oblique:
      this->ObliquePlane->GetOrigin(origin);
      break;
  }
}

//----------------------------------------------------------------------
// Calculate the distance of a point from the Object. Negative
// values imply that the point is outside. Positive values imply that it is
// inside. The closest point to the object is returned in closestPt.
double svtkBoundedPlanePointPlacer ::GetDistanceFromObject(
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
void svtkBoundedPlanePointPlacer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Projection Normal: ";
  if (this->ProjectionNormal == svtkBoundedPlanePointPlacer::XAxis)
  {
    os << "XAxis\n";
  }
  else if (this->ProjectionNormal == svtkBoundedPlanePointPlacer::YAxis)
  {
    os << "YAxis\n";
  }
  else if (this->ProjectionNormal == svtkBoundedPlanePointPlacer::ZAxis)
  {
    os << "ZAxis\n";
  }
  else // if ( this->ProjectionNormal == svtkBoundedPlanePointPlacer::Oblique )
  {
    os << "Oblique\n";
  }

  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";

  os << indent << "Bounding Planes:\n";
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << " (none)\n";
  }
  os << indent << "Oblique plane:\n";
  if (this->ObliquePlane)
  {
    this->ObliquePlane->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << " (none)\n";
  }
}
