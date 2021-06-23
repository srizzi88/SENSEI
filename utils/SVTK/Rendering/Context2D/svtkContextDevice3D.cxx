/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextDevice3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextDevice3D.h"
#include "svtkObjectFactory.h"

svtkAbstractObjectFactoryNewMacro(svtkContextDevice3D);

svtkContextDevice3D::svtkContextDevice3D() = default;

svtkContextDevice3D::~svtkContextDevice3D() = default;

//-----------------------------------------------------------------------------
void svtkContextDevice3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
