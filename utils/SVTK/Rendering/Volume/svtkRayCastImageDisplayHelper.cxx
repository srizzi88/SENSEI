/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRayCastImageDisplayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRayCastImageDisplayHelper.h"
#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
svtkAbstractObjectFactoryNewMacro(svtkRayCastImageDisplayHelper);
//----------------------------------------------------------------------------

// Construct a new svtkRayCastImageDisplayHelper with default values
svtkRayCastImageDisplayHelper::svtkRayCastImageDisplayHelper()
{
  this->PreMultipliedColors = 1;
  this->PixelScale = 1.0;
}

// Destruct a svtkRayCastImageDisplayHelper - clean up any memory used
svtkRayCastImageDisplayHelper::~svtkRayCastImageDisplayHelper() = default;

void svtkRayCastImageDisplayHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PreMultiplied Colors: " << (this->PreMultipliedColors ? "On" : "Off") << endl;

  os << indent << "Pixel Scale: " << this->PixelScale << endl;
}
