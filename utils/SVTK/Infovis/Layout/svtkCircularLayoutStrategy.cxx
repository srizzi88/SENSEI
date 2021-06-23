/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCircularLayoutStrategy.cxx

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

#include "svtkCircularLayoutStrategy.h"

#include "svtkGraph.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"

svtkStandardNewMacro(svtkCircularLayoutStrategy);

svtkCircularLayoutStrategy::svtkCircularLayoutStrategy() = default;

svtkCircularLayoutStrategy::~svtkCircularLayoutStrategy() = default;

void svtkCircularLayoutStrategy::Layout()
{
  svtkPoints* points = svtkPoints::New();
  svtkIdType numVerts = this->Graph->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for (svtkIdType i = 0; i < numVerts; i++)
  {
    double x = cos(2.0 * svtkMath::Pi() * i / numVerts);
    double y = sin(2.0 * svtkMath::Pi() * i / numVerts);
    points->SetPoint(i, x, y, 0);
  }
  this->Graph->SetPoints(points);
  points->Delete();
}

void svtkCircularLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
