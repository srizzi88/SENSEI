/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipConvexPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClipConvexPolyData.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPolyData.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

svtkStandardNewMacro(svtkClipConvexPolyData);

svtkCxxSetObjectMacro(svtkClipConvexPolyData, Planes, svtkPlaneCollection);

// ----------------------------------------------------------------------------
class svtkCCPDVertex
{
public:
  double Point[3];
};

// ----------------------------------------------------------------------------
class svtkCCPDPolygon
{
public:
  std::vector<svtkCCPDVertex*> Vertices;
  std::vector<svtkCCPDVertex*> NewVertices;
};

// ----------------------------------------------------------------------------
class svtkClipConvexPolyDataInternals
{
public:
  std::vector<svtkCCPDPolygon*> Polygons;
};

// ----------------------------------------------------------------------------
// Constructor
svtkClipConvexPolyData::svtkClipConvexPolyData()
{
  this->Planes = nullptr;
  this->Internal = new svtkClipConvexPolyDataInternals;
}

// ----------------------------------------------------------------------------
// Destructor
svtkClipConvexPolyData::~svtkClipConvexPolyData()
{
  this->SetPlanes(nullptr);
  this->ClearInternals();
  delete this->Internal;
}

// ----------------------------------------------------------------------------
// Description:
// Redefines this method, as this filter depends on time of its components
// (planes)
svtkMTimeType svtkClipConvexPolyData::GetMTime()
{
  svtkMTimeType result = Superclass::GetMTime();
  if (this->Planes != nullptr)
  {
    svtkMTimeType planesTime = this->Planes->GetMTime();
    if (planesTime > result)
    {
      result = planesTime;
    }
  }
  return result;
}

// ----------------------------------------------------------------------------
void svtkClipConvexPolyData::ClearInternals()
{
  unsigned int j;
  for (unsigned int i = 0; i < this->Internal->Polygons.size(); i++)
  {
    for (j = 0; j < this->Internal->Polygons[i]->Vertices.size(); j++)
    {
      delete this->Internal->Polygons[i]->Vertices[j];
    }
    this->Internal->Polygons[i]->Vertices.clear();

    for (j = 0; j < this->Internal->Polygons[i]->NewVertices.size(); j++)
    {
      delete this->Internal->Polygons[i]->NewVertices[j];
    }
    this->Internal->Polygons[i]->NewVertices.clear();

    delete this->Internal->Polygons[i];
  }
  this->Internal->Polygons.clear();
}

// ----------------------------------------------------------------------------
void svtkClipConvexPolyData::ClearNewVertices()
{
  for (unsigned int i = 0; i < this->Internal->Polygons.size(); i++)
  {
    for (unsigned int j = 0; j < this->Internal->Polygons[i]->NewVertices.size(); j++)
    {
      delete this->Internal->Polygons[i]->NewVertices[j];
    }
    this->Internal->Polygons[i]->NewVertices.clear();
  }
}

// ----------------------------------------------------------------------------
void svtkClipConvexPolyData::RemoveEmptyPolygons()
{
  bool done = false;

  while (!done)
  {
    done = true;
    for (unsigned int i = 0; i < this->Internal->Polygons.size(); i++)
    {
      if (this->Internal->Polygons[i]->Vertices.empty())
      {
        std::vector<svtkCCPDPolygon*>::iterator where = std::find(this->Internal->Polygons.begin(),
          this->Internal->Polygons.end(), this->Internal->Polygons[i]);
        if (where != this->Internal->Polygons.end())
        {
          delete this->Internal->Polygons[i];
          this->Internal->Polygons.erase(where);
          done = false;
          break;
        }
      }
    }
  }
}

// ----------------------------------------------------------------------------
//
int svtkClipConvexPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Pre-conditions
  if (this->Planes == nullptr)
  {
    svtkErrorMacro("plane collection is null");
    return 0;
  }
  if (this->Planes->GetNumberOfItems() == 0)
  {
    svtkErrorMacro("plane collection is empty");
    return 0;
  }

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkCellArray* polys = input->GetPolys();
  svtkPoints* points = input->GetPoints();

  // Compute tolerance to be 0.00001 of diagonal
  double min[3] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MAX };
  double max[3] = { SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MIN };

  size_t i, j;
  double tolerance;
  for (i = 0; i < static_cast<size_t>(points->GetNumberOfPoints()); i++)
  {
    double pt[3];
    points->GetPoint(static_cast<svtkIdType>(i), pt);
    for (j = 0; j < 3; j++)
    {
      min[j] = (pt[j] < min[j]) ? (pt[j]) : (min[j]);
      max[j] = (pt[j] > max[j]) ? (pt[j]) : (max[j]);
    }
  }
  tolerance = sqrt(svtkMath::Distance2BetweenPoints(min, max)) * 0.00001;

  // Copy the polygons from the polys array to the internal
  // data structure
  svtkIdType npts;
  const svtkIdType* pts;
  polys->InitTraversal();
  while (polys->GetNextCell(npts, pts))
  {
    svtkCCPDPolygon* polygon = new svtkCCPDPolygon;
    for (i = 0; i < static_cast<size_t>(npts); i++)
    {
      svtkCCPDVertex* v = new svtkCCPDVertex;
      points->GetPoint(pts[i], v->Point);
      polygon->Vertices.push_back(v);
    }
    this->Internal->Polygons.push_back(polygon);
  }

  this->Planes->InitTraversal();
  svtkPlane* plane;

  // For each plane in the collection, clip the polygons with the plane.
  while ((plane = this->Planes->GetNextItem()))
  {
    if (!this->HasDegeneracies(plane))
    {
      this->ClipWithPlane(plane, tolerance);
    }
  }

  // Create a new set of points and polygons into which the results will
  // be stored
  svtkPoints* outPoints = svtkPoints::New();
  svtkCellArray* outPolys = svtkCellArray::New();

  std::vector<svtkIdType> polyPts(32);
  for (i = 0; i < this->Internal->Polygons.size(); i++)
  {
    size_t numPoints = this->Internal->Polygons[i]->Vertices.size();
    if (numPoints > polyPts.size())
    {
      polyPts.resize(numPoints);
    }
    for (j = 0; j < numPoints; j++)
    {
      polyPts[j] = outPoints->InsertNextPoint(this->Internal->Polygons[i]->Vertices[j]->Point);
    }
    outPolys->InsertNextCell(static_cast<svtkIdType>(numPoints), polyPts.data());
  }

  // Set the output vertices and polygons
  output->SetPoints(outPoints);
  output->SetPolys(outPolys);

  // Delete the temporary storage
  outPoints->Delete();
  outPolys->Delete();

  this->ClearInternals();

  return 1;
}

// ----------------------------------------------------------------------------
void svtkClipConvexPolyData::ClipWithPlane(svtkPlane* plane, double tolerance)
{
  double origin[3];
  double normal[4];

  plane->GetOrigin(origin);
  plane->GetNormal(normal);

  svtkMath::Normalize(normal);

  normal[3] = -(origin[0] * normal[0] + origin[1] * normal[1] + origin[2] * normal[2]);

  int numNewPoints = 0;
  unsigned int i;
  size_t j;

  // For each polygon
  for (i = 0; i < this->Internal->Polygons.size(); i++)
  {
    // the new polygon. We are clipping the existing polygon then
    // removing that old polygon and adding this clipped polygon
    svtkCCPDPolygon* newPoly = new svtkCCPDPolygon;

    int somePositive = 0;
    // Look around the polygon - we only want to process it if
    // there are some positive vertices when passed through the
    // plane equation. If they are all negative, then it is entirely
    // clipped. If they are either negative or 0, then this is a boundary
    // condition and we also don't want to consider it
    size_t numVertices = this->Internal->Polygons[i]->Vertices.size();
    for (j = 0; j < numVertices; j++)
    {
      double* p1 = this->Internal->Polygons[i]->Vertices[j]->Point;
      double p1D = p1[0] * normal[0] + p1[1] * normal[1] + p1[2] * normal[2] + normal[3];
      if (p1D < 2.0 * tolerance && p1D > -2.0 * tolerance)
      {
        p1D = 0;
      }
      if (p1D > 0)
      {
        somePositive = 1;
        break;
      }
    }

    if (somePositive)
    {
      // For each vertex
      for (j = 0; j < numVertices; j++)
      {
        double* p1 = this->Internal->Polygons[i]->Vertices[j]->Point;
        double* p2 = this->Internal->Polygons[i]->Vertices[(j + 1) % numVertices]->Point;

        double p1D = p1[0] * normal[0] + p1[1] * normal[1] + p1[2] * normal[2] + normal[3];
        double p2D = p2[0] * normal[0] + p2[1] * normal[1] + p2[2] * normal[2] + normal[3];

        // We want to avoid the case where we just barely clip a vertex. If we allow
        // that to happen then we wind up with too many candidate points all in
        // approximately the same place when we try to form a loop to close off
        // the cut. So if the point is within a 1/10th of the tolerance factor
        // we've set, we'll just consider it not clipped.
        if (p1D < 2 * tolerance && p1D > -2.0 * tolerance)
        {
          p1D = 0;
        }

        if (p2D < 2 * tolerance && p2D > -2.0 * tolerance)
        {
          p2D = 0;
        }
        // Add p1 in if it is not clipped. If the whole polygon is unclipped
        // then we'll just add in each vertex in turn. If the whole polygon
        // is clipped we won't add in any vertices. If the polygon is
        // clipped, we'll add in two new points corresponding the
        // crossing location of the plane on two edges of the polygon
        if (p1D >= 0)
        {
          svtkCCPDVertex* v = new svtkCCPDVertex;
          v->Point[0] = p1[0];
          v->Point[1] = p1[1];
          v->Point[2] = p1[2];
          newPoly->Vertices.push_back(v);
        }

        // If the first point is exactly on the boundary, we need to also count it
        // as a new point
        if (p1D == 0 && p2D <= 0)
        {
          svtkCCPDVertex* v = new svtkCCPDVertex;
          v->Point[0] = p1[0];
          v->Point[1] = p1[1];
          v->Point[2] = p1[2];
          this->Internal->Polygons[i]->NewVertices.push_back(v);
          numNewPoints++;
        }

        if (p2D == 0 && p1D <= 0)
        {
          svtkCCPDVertex* v = new svtkCCPDVertex;
          v->Point[0] = p2[0];
          v->Point[1] = p2[1];
          v->Point[2] = p2[2];
          this->Internal->Polygons[i]->NewVertices.push_back(v);
          numNewPoints++;
        }

        // If the plane clips this edge - find the crossing point. We'll need
        // to add this point to the new polygon
        if (p1D * p2D < 0)
        {
          double w = -p1D / (p2D - p1D);

          svtkCCPDVertex* v = new svtkCCPDVertex;
          v->Point[0] = p1[0] + w * (p2[0] - p1[0]);
          v->Point[1] = p1[1] + w * (p2[1] - p1[1]);
          v->Point[2] = p1[2] + w * (p2[2] - p1[2]);
          newPoly->Vertices.push_back(v);

          v = new svtkCCPDVertex;
          v->Point[0] = p1[0] + w * (p2[0] - p1[0]);
          v->Point[1] = p1[1] + w * (p2[1] - p1[1]);
          v->Point[2] = p1[2] + w * (p2[2] - p1[2]);
          this->Internal->Polygons[i]->NewVertices.push_back(v);
          numNewPoints++;
        }
      }
    }

    // Remove the current polygon
    for (j = 0; j < numVertices; j++)
    {
      delete this->Internal->Polygons[i]->Vertices[j];
    }
    this->Internal->Polygons[i]->Vertices.clear();

    // copy in the new polygon if it isn't entirely clipped
    numVertices = newPoly->Vertices.size();
    if (numVertices > 0)
    {
      for (j = 0; j < numVertices; j++)
      {
        this->Internal->Polygons[i]->Vertices.push_back(newPoly->Vertices[j]);
      }
      newPoly->Vertices.clear();
    }
    delete newPoly;
  }

  // If we've added any new points when clipping the polydata then
  // we must have added at least six. Otherwise something is wrong
  if (numNewPoints)
  {
    if (numNewPoints < 6)
    {
      svtkErrorMacro(<< "Failure - not enough new points");
      return;
    }

    // Check that all new arrays contain exactly 0 or 2 points
    for (i = 0; i < this->Internal->Polygons.size(); i++)
    {
      if (!this->Internal->Polygons[i]->NewVertices.empty() &&
        this->Internal->Polygons[i]->NewVertices.size() != 2)
      {
        svtkErrorMacro(<< "Horrible error - we have "
                      << this->Internal->Polygons[i]->NewVertices.size() << " crossing points");
        return;
      }
    }

    // Find the first polygon with a new point
    size_t idx = 0;
    bool idxFound = false;

    for (i = 0; !idxFound && i < this->Internal->Polygons.size(); i++)
    {
      idxFound = !this->Internal->Polygons[i]->NewVertices.empty();
      if (idxFound)
      {
        idx = i;
      }
    }

    if (!idxFound)
    {
      svtkErrorMacro(<< "Couldn't find any new vertices!");
      return;
    }

    // the new polygon
    svtkCCPDPolygon* newPoly = new svtkCCPDPolygon;
    svtkCCPDVertex* v = new svtkCCPDVertex;
    v->Point[0] = this->Internal->Polygons[idx]->NewVertices[0]->Point[0];
    v->Point[1] = this->Internal->Polygons[idx]->NewVertices[0]->Point[1];
    v->Point[2] = this->Internal->Polygons[idx]->NewVertices[0]->Point[2];
    newPoly->Vertices.push_back(v);

    v = new svtkCCPDVertex;
    v->Point[0] = this->Internal->Polygons[idx]->NewVertices[1]->Point[0];
    v->Point[1] = this->Internal->Polygons[idx]->NewVertices[1]->Point[1];
    v->Point[2] = this->Internal->Polygons[idx]->NewVertices[1]->Point[2];
    newPoly->Vertices.push_back(v);

    double lastPoint[3];
    lastPoint[0] = this->Internal->Polygons[idx]->NewVertices[1]->Point[0];
    lastPoint[1] = this->Internal->Polygons[idx]->NewVertices[1]->Point[1];
    lastPoint[2] = this->Internal->Polygons[idx]->NewVertices[1]->Point[2];

    size_t lastPointIdx = idx;
    size_t subIdx;

    while (static_cast<int>(newPoly->Vertices.size()) < numNewPoints / 2)
    {
      // Find the index of the closest new vertex that matches the
      // lastPoint but not the lastPointIdx.
      subIdx = 0;
      float closestDistance = SVTK_FLOAT_MAX;
      bool foundSubIdx = false;
      for (i = 0; i < this->Internal->Polygons.size(); i++)
      {
        if (i != lastPointIdx && !this->Internal->Polygons[i]->NewVertices.empty())
        {
          for (j = 0; j < 2; j++)
          {
            float testDistance = svtkMath::Distance2BetweenPoints(
              lastPoint, this->Internal->Polygons[i]->NewVertices[j]->Point);
            if (testDistance < tolerance && testDistance < closestDistance)
            {
              closestDistance = testDistance;
              idx = i;
              subIdx = j;
              foundSubIdx = true;
            }
          }
        }
      }

      if (!foundSubIdx)
      {
        svtkErrorMacro("Could not find a match");
      }

      v = new svtkCCPDVertex;
      v->Point[0] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[0];
      v->Point[1] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[1];
      v->Point[2] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[2];
      newPoly->Vertices.push_back(v);

      lastPoint[0] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[0];
      lastPoint[1] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[1];
      lastPoint[2] = this->Internal->Polygons[idx]->NewVertices[(subIdx + 1) % 2]->Point[2];
      lastPointIdx = idx;
    }

    // check to see that the polygon vertices are in the right order.
    // cross product of p1p2 and p3p2 should point in the same direction
    // as the plane normal. Otherwise, reverse the order
    int flipCount = 0;
    int checkCount = 0;
    for (size_t startV = 0; startV + 2 < newPoly->Vertices.size(); startV++)
    {
      double* p1 = newPoly->Vertices[startV]->Point;
      double* p2 = newPoly->Vertices[startV + 1]->Point;
      double* p3 = newPoly->Vertices[startV + 2]->Point;
      double v1[3];
      double v2[3];
      double cross[3];
      v1[0] = p1[0] - p2[0];
      v1[1] = p1[1] - p2[1];
      v1[2] = p1[2] - p2[2];
      v2[0] = p3[0] - p2[0];
      v2[1] = p3[1] - p2[1];
      v2[2] = p3[2] - p2[2];
      svtkMath::Cross(v1, v2, cross);
      double nd = svtkMath::Normalize(cross);
      // only check if the length of the cross product is long
      // enough - otherwise we might be working with points that
      // are all too close together and we might wind up with
      // misleading results
      if (nd > tolerance)
      {
        if (svtkMath::Dot(cross, normal) < 0)
        {
          flipCount++;
        }
        checkCount++;
      }
    }

    // As long as more than half the time we checked we got the answer
    // that we should flip, go ahead and flip it
    if (flipCount > checkCount / 2)
    {
      std::reverse(newPoly->Vertices.begin(), newPoly->Vertices.end());
    }
    this->Internal->Polygons.push_back(newPoly);
  }
  this->RemoveEmptyPolygons();
  this->ClearNewVertices();
}

// ----------------------------------------------------------------------------
bool svtkClipConvexPolyData::HasDegeneracies(svtkPlane* plane)
{
  double origin[3];
  double normal[4];

  plane->GetOrigin(origin);
  plane->GetNormal(normal);

  normal[3] = -(origin[0] * normal[0] + origin[1] * normal[1] + origin[2] * normal[2]);

  unsigned int i;
  size_t j;
  // For each polygon
  int totalNumNewVertices = 0;
  for (i = 0; i < this->Internal->Polygons.size(); i++)
  {
    // For each vertex
    size_t numVertices = this->Internal->Polygons[i]->Vertices.size();
    int numNewVertices = 0;
    for (j = 0; j < numVertices; j++)
    {
      double* p1 = this->Internal->Polygons[i]->Vertices[j]->Point;
      double* p2 = this->Internal->Polygons[i]->Vertices[(j + 1) % numVertices]->Point;

      double p1D = p1[0] * normal[0] + p1[1] * normal[1] + p1[2] * normal[2] + normal[3];
      double p2D = p2[0] * normal[0] + p2[1] * normal[1] + p2[2] * normal[2] + normal[3];

      // If the plane clips this edge - find the crossing point
      if (p1D * p2D <= 0)
      {
        numNewVertices++;
      }
    }
    if (numNewVertices != 0 && numNewVertices != 2)
    {
      return true;
    }
    totalNumNewVertices += numNewVertices;
  }

  if (totalNumNewVertices < 6)
  {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
void svtkClipConvexPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Planes: " << this->Planes << endl;
}
