/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomLayoutStrategy.cxx

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

#include "svtkRandomLayoutStrategy.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkRandomLayoutStrategy);

svtkRandomLayoutStrategy::svtkRandomLayoutStrategy()
{
  this->RandomSeed = 123;
  this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
  this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] = 0.5;
  this->AutomaticBoundsComputation = 0;
  this->ThreeDimensionalLayout = 1;
}

svtkRandomLayoutStrategy::~svtkRandomLayoutStrategy() = default;

// Random graph layout method
// Fixme: Temporary Hack
void svtkRandomLayoutStrategy::Layout() {}

void svtkRandomLayoutStrategy::SetGraph(svtkGraph* graph)
{
  if (graph == nullptr)
  {
    return;
  }

  // Generate bounds automatically if necessary. It's the same
  // as the graph bounds.
  if (this->AutomaticBoundsComputation)
  {
    svtkPoints* pts = graph->GetPoints();
    pts->GetBounds(this->GraphBounds);
  }

  for (int i = 0; i < 3; i++)
  {
    if (this->GraphBounds[2 * i + 1] <= this->GraphBounds[2 * i])
    {
      this->GraphBounds[2 * i + 1] = this->GraphBounds[2 * i] + 1;
    }
  }

  // Generate the points, either x,y,0 or x,y,z
  svtkMath::RandomSeed(this->RandomSeed);

  svtkPoints* newPoints = svtkPoints::New();
  for (int i = 0; i < graph->GetNumberOfVertices(); i++)
  {
    double x, y, z, r;
    r = svtkMath::Random();
    x = (this->GraphBounds[1] - this->GraphBounds[0]) * r + this->GraphBounds[0];
    r = svtkMath::Random();
    y = (this->GraphBounds[3] - this->GraphBounds[2]) * r + this->GraphBounds[2];
    if (this->ThreeDimensionalLayout)
    {
      r = svtkMath::Random();
      z = (this->GraphBounds[5] - this->GraphBounds[4]) * r + this->GraphBounds[4];
    }
    else
    {
      z = 0;
    }
    newPoints->InsertNextPoint(x, y, z);
  }

  // Set the graph points.
  graph->SetPoints(newPoints);

  // Clean up.
  newPoints->Delete();
}

void svtkRandomLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RandomSeed: " << this->RandomSeed << endl;

  os << indent
     << "AutomaticBoundsComputation: " << (this->AutomaticBoundsComputation ? "On\n" : "Off\n");

  os << indent << "GraphBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->GraphBounds[0] << ", " << this->GraphBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->GraphBounds[2] << ", " << this->GraphBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->GraphBounds[4] << ", " << this->GraphBounds[5] << ")\n";

  os << indent << "Three Dimensional Layout: " << (this->ThreeDimensionalLayout ? "On\n" : "Off\n");
}
