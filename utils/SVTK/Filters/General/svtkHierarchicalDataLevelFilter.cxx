/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalDataLevelFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHierarchicalDataLevelFilter.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkHierarchicalDataLevelFilter);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
svtkHierarchicalDataLevelFilter::svtkHierarchicalDataLevelFilter() = default;

svtkHierarchicalDataLevelFilter::~svtkHierarchicalDataLevelFilter() = default;

void svtkHierarchicalDataLevelFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
