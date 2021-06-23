/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphGeodesicPath.h"

#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkGraphGeodesicPath::svtkGraphGeodesicPath()
{
  this->StartVertex = 0;
  this->EndVertex = 0;
}

//-----------------------------------------------------------------------------
svtkGraphGeodesicPath::~svtkGraphGeodesicPath() = default;

//-----------------------------------------------------------------------------
void svtkGraphGeodesicPath::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StartVertex: " << this->StartVertex << endl;
  os << indent << "EndVertex: " << this->EndVertex << endl;
}
