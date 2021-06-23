/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLogLookupTable.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkLogLookupTable);

// Construct with (minimum,maximum) range 1 to 10 (based on
// logarithmic values).
svtkLogLookupTable::svtkLogLookupTable(int sze, int ext)
  : svtkLookupTable(sze, ext)
{
  this->Scale = SVTK_SCALE_LOG10;

  this->TableRange[0] = 1;
  this->TableRange[1] = 10;
}

void svtkLogLookupTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
