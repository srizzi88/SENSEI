/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoEdgeStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkGeoEdgeStrategy.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkEdgeListIterator.h"
#include "svtkFloatArray.h"
#include "svtkGeoMath.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkGeoEdgeStrategy);

svtkGeoEdgeStrategy::svtkGeoEdgeStrategy()
{
  this->GlobeRadius = svtkGeoMath::EarthRadiusMeters();
  this->ExplodeFactor = 0.2;
  this->NumberOfSubdivisions = 20;
}

void svtkGeoEdgeStrategy::Layout()
{
  std::map<std::pair<svtkIdType, svtkIdType>, int> edgeCount;
  std::map<std::pair<svtkIdType, svtkIdType>, int> edgeNumber;
  std::vector<svtkEdgeType> edgeVector(this->Graph->GetNumberOfEdges());
  svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
  this->Graph->GetEdges(it);
  while (it->HasNext())
  {
    svtkEdgeType e = it->Next();
    svtkIdType src, tgt;
    if (e.Source < e.Target)
    {
      src = e.Source;
      tgt = e.Target;
    }
    else
    {
      src = e.Target;
      tgt = e.Source;
    }
    edgeCount[std::pair<svtkIdType, svtkIdType>(src, tgt)]++;
    edgeVector[e.Id] = e;
  }
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  double* pts = new double[this->NumberOfSubdivisions * 3];
  for (svtkIdType eid = 0; eid < numEdges; ++eid)
  {
    svtkEdgeType e = edgeVector[eid];
    svtkIdType src, tgt;
    if (e.Source < e.Target)
    {
      src = e.Source;
      tgt = e.Target;
    }
    else
    {
      src = e.Target;
      tgt = e.Source;
    }
    // Lookup the total number of edges with this source
    // and target, as well as how many times this pair
    // has been found so far.
    std::pair<svtkIdType, svtkIdType> p(src, tgt);
    edgeNumber[p]++;
    int cur = edgeNumber[p];
    int total = edgeCount[p];

    double sourcePt[3];
    double targetPt[3];
    this->Graph->GetPoint(e.Source, sourcePt);
    this->Graph->GetPoint(e.Target, targetPt);

    // Find w, a unit vector pointing from the center of the
    // earth directly between the two endpoints.
    double w[3];
    for (int c = 0; c < 3; ++c)
    {
      w[c] = (sourcePt[c] + targetPt[c]) / 2.0;
    }
    svtkMath::Normalize(w);

    // The center of the circle used to draw the arc is a
    // point along the vector w scaled by the explode factor.
    // Use cur and total to separate parallel arcs.
    double center[3];
    for (int c = 0; c < 3; ++c)
    {
      center[c] = this->ExplodeFactor * this->GlobeRadius * w[c] * (cur + 1) / total;
    }

    // The vectors u and x are unit vectors pointing from the
    // center of the circle to the two endpoints of the arc,
    // lastPoint and curPoint, respectively.
    double u[3], x[3];
    for (int c = 0; c < 3; ++c)
    {
      u[c] = sourcePt[c] - center[c];
      x[c] = targetPt[c] - center[c];
    }
    double radius = svtkMath::Norm(u);
    svtkMath::Normalize(u);
    svtkMath::Normalize(x);

    // Find the angle that the arc spans.
    double theta = acos(svtkMath::Dot(u, x));

    // If the vectors u, x point toward the center of the earth, take
    // the larger angle between the vectors.
    // We determine whether u points toward the center of the earth
    // by checking whether the dot product of u and w is negative.
    if (svtkMath::Dot(w, u) < 0)
    {
      theta = 2.0 * svtkMath::Pi() - theta;
    }

    // We need two perpendicular vectors on the plane of the circle
    // in order to draw the circle.  First we calculate n, a vector
    // normal to the circle, by crossing u and w.  Next, we cross
    // n and u in order to get a vector v in the plane of the circle
    // that is perpendicular to u.
    double n[3];
    svtkMath::Cross(u, w, n);
    svtkMath::Normalize(n);
    double v[3];
    svtkMath::Cross(n, u, v);
    svtkMath::Normalize(v);

    // Use the general equation for a circle in three dimensions
    // to draw an arc from the last point to the current point.
    for (int s = 0; s < this->NumberOfSubdivisions; ++s)
    {
      double angle =
        (this->NumberOfSubdivisions - 1.0 - s) * theta / (this->NumberOfSubdivisions - 1.0);
      for (int c = 0; c < 3; ++c)
      {
        pts[3 * s + c] = center[c] + radius * cos(angle) * u[c] + radius * sin(angle) * v[c];
      }
    }
    this->Graph->SetEdgePoints(e.Id, this->NumberOfSubdivisions, pts);

    if (eid % 1000 == 0)
    {
      double progress = eid / static_cast<double>(numEdges);
      this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&progress));
    }
  }
  double progress = 1.0;
  this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&progress));
  delete[] pts;
}

void svtkGeoEdgeStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "ExplodeFactor: " << this->ExplodeFactor << endl;
  os << indent << "NumberOfSubdivisions: " << this->NumberOfSubdivisions << endl;
}
