/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThroughEdgeStrategy.cxx

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
#include "svtkPassThroughEdgeStrategy.h"

#include "svtkCellArray.h"
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

svtkStandardNewMacro(svtkPassThroughEdgeStrategy);

svtkPassThroughEdgeStrategy::svtkPassThroughEdgeStrategy() = default;

svtkPassThroughEdgeStrategy::~svtkPassThroughEdgeStrategy() = default;

void svtkPassThroughEdgeStrategy::Layout() {}

void svtkPassThroughEdgeStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
