/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWeightEuclideanDistanceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphWeightEuclideanDistanceFilter.h"

#include "svtkGraph.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkGraphWeightEuclideanDistanceFilter);

float svtkGraphWeightEuclideanDistanceFilter::ComputeWeight(
  svtkGraph* const graph, const svtkEdgeType& edge) const
{
  double p1[3];
  graph->GetPoint(edge.Source, p1);

  double p2[3];
  graph->GetPoint(edge.Target, p2);

  float w = static_cast<float>(sqrt(svtkMath::Distance2BetweenPoints(p1, p2)));

  return w;
}

bool svtkGraphWeightEuclideanDistanceFilter::CheckRequirements(svtkGraph* const graph) const
{
  svtkPoints* points = graph->GetPoints();
  if (!points)
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void svtkGraphWeightEuclideanDistanceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkGraphWeightFilter::PrintSelf(os, indent);
}
