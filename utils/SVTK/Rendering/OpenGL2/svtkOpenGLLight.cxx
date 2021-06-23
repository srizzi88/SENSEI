/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLLight.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkOpenGLLight);

// Implement base class method.
void svtkOpenGLLight::Render(svtkRenderer* svtkNotUsed(ren), int svtkNotUsed(light_index))
{
  // all handled by the mappers
}

//----------------------------------------------------------------------------
void svtkOpenGLLight::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
