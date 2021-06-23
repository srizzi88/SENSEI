/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArcParallelEdgeStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkArcParallelEdgeStrategy.h"

#include "svtkCellArray.h"
#include "svtkCommand.h"
#include "svtkDirectedGraph.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"

#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkArcParallelEdgeStrategy);

svtkArcParallelEdgeStrategy::svtkArcParallelEdgeStrategy()
{
  this->NumberOfSubdivisions = 10;
}

svtkArcParallelEdgeStrategy::~svtkArcParallelEdgeStrategy() = default;

void svtkArcParallelEdgeStrategy::Layout()
{
  bool directed = svtkDirectedGraph::SafeDownCast(this->Graph) != nullptr;
  std::map<std::pair<svtkIdType, svtkIdType>, int> edgeCount;
  std::map<std::pair<svtkIdType, svtkIdType>, int> edgeNumber;
  std::vector<svtkEdgeType> edgeVector(this->Graph->GetNumberOfEdges());
  svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
  this->Graph->GetEdges(it);
  double avgEdgeLength = 0.0;
  while (it->HasNext())
  {
    svtkEdgeType e = it->Next();
    svtkIdType src, tgt;
    if (directed || e.Source < e.Target)
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

    // Compute edge length
    double sourcePt[3];
    double targetPt[3];
    this->Graph->GetPoint(e.Source, sourcePt);
    this->Graph->GetPoint(e.Target, targetPt);
    avgEdgeLength += sqrt(svtkMath::Distance2BetweenPoints(sourcePt, targetPt));
  }
  svtkIdType numEdges = this->Graph->GetNumberOfEdges();
  if (numEdges > 0)
  {
    avgEdgeLength /= numEdges;
  }
  else
  {
    avgEdgeLength = 1.0;
  }
  double maxLoopHeight = avgEdgeLength / 10.0;
  double* pts = new double[this->NumberOfSubdivisions * 3];
  for (svtkIdType eid = 0; eid < numEdges; ++eid)
  {
    svtkEdgeType e = edgeVector[eid];
    svtkIdType src, tgt;
    if (directed || e.Source < e.Target)
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
    std::pair<svtkIdType, svtkIdType> revP(tgt, src);
    int revTotal = edgeCount[revP];

    double sourcePt[3];
    double targetPt[3];
    this->Graph->GetPoint(e.Source, sourcePt);
    this->Graph->GetPoint(e.Target, targetPt);

    // If only one edge between source and target,
    // just draw a straight line.
    if (total + revTotal == 1)
    {
      double pt[6];
      pt[0] = sourcePt[0];
      pt[1] = sourcePt[1];
      pt[2] = sourcePt[2];
      pt[3] = targetPt[0];
      pt[4] = targetPt[1];
      pt[5] = targetPt[2];
      this->Graph->SetEdgePoints(e.Id, 2, pt);
      continue;
    }

    // Find vector from source to target
    double delta[3];
    for (int c = 0; c < 3; ++c)
    {
      delta[c] = targetPt[c] - sourcePt[c];
    }
    double dist = svtkMath::Norm(delta);

    // If the distance is zero, draw a loop.
    if (dist == 0)
    {
      double radius = maxLoopHeight * cur / total;
      double u[3] = { 1.0, 0.0, 0.0 };
      double v[3] = { 0.0, 1.0, 0.0 };
      double center[3] = { sourcePt[0] - radius, sourcePt[1], sourcePt[2] };
      // Use the general equation for a circle in three dimensions
      // to draw a loop.
      for (int s = 0; s < this->NumberOfSubdivisions; ++s)
      {
        double angle = 2.0 * svtkMath::Pi() * s / (this->NumberOfSubdivisions - 1);
        for (int c = 0; c < 3; ++c)
        {
          pts[3 * s + c] =
            center[c] + radius * cos(angle) * u[c] + radius / 2.0 * sin(angle) * v[c];
        }
      }
      this->Graph->SetEdgePoints(e.Id, this->NumberOfSubdivisions, pts);
      continue;
    }

    // Find vector perpendicular to delta
    // and (0,0,1).
    double z[3] = { 0.0, 0.0, 1.0 };
    double w[3];
    svtkMath::Cross(delta, z, w);
    svtkMath::Normalize(w);

    // Really bad ascii art:
    //    ___-------___
    //   /      |height\ <-- the drawn arc
    // src----dist-----tgt
    //   \      |      /
    //    \     |offset
    //     \    |    /
    //    u \   |   / x
    //       \  |  /
    //        \ | /
    //         \|/
    //        center
    // The center of the circle used to draw the arc is a
    // point along the vector w a certain distance (offset)
    // from the midpoint of sourcePt and targetPt.
    // The offset is computed to give a certain arc height
    // based on cur and total.
    double maxHeight = dist / 8.0;
    double height;
    int sign = 1;
    if (directed)
    {
      // Directed edges will go on one side or the other
      // automatically based on the order of source and target.
      height = (static_cast<double>(cur) / total) * maxHeight;
    }
    else
    {
      // For undirected edges, place every other edge on one
      // side or the other.
      height = (static_cast<double>((cur + 1) / 2) / (total / 2)) * maxHeight;
      if (cur % 2)
      {
        sign = -1;
      }
    }
    // This formula computes offset given dist and height.
    // You can pull out your trig formulas and verify it :)
    double offset = (dist * dist / 4.0 - height * height) / (2.0 * height);
    double center[3];
    for (int c = 0; c < 3; ++c)
    {
      center[c] = (targetPt[c] + sourcePt[c]) / 2.0 + sign * offset * w[c];
    }

    // The vectors u and x are unit vectors pointing from the
    // center of the circle to the two endpoints of the arc,
    // sourcePt and targetPt, respectively.
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
      double angle = -sign * s * theta / (this->NumberOfSubdivisions - 1.0);
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

void svtkArcParallelEdgeStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfSubdivisions: " << this->NumberOfSubdivisions << endl;
}
