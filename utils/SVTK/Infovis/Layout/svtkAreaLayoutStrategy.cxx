/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaLayoutStrategy.cxx

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

#include "svtkAreaLayoutStrategy.h"

#include "svtkTree.h"

svtkAreaLayoutStrategy::svtkAreaLayoutStrategy()
{
  this->ShrinkPercentage = 0.0;
}

svtkAreaLayoutStrategy::~svtkAreaLayoutStrategy() = default;

void svtkAreaLayoutStrategy::LayoutEdgePoints(svtkTree* inputTree,
  svtkDataArray* svtkNotUsed(coordsArray), svtkDataArray* svtkNotUsed(sizeArray),
  svtkTree* edgeRoutingTree)
{
  edgeRoutingTree->ShallowCopy(inputTree);
}

void svtkAreaLayoutStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
