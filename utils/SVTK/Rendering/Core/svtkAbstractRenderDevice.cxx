/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractRenderDevice.h"
#include "svtkObjectFactory.h"

svtkAbstractObjectFactoryNewMacro(svtkAbstractRenderDevice);

svtkAbstractRenderDevice::svtkAbstractRenderDevice()
  : GLMajor(2)
  , GLMinor(1)
{
}

svtkAbstractRenderDevice::~svtkAbstractRenderDevice() = default;

void svtkAbstractRenderDevice::SetRequestedGLVersion(int major, int minor)
{
  this->GLMajor = major;
  this->GLMinor = minor;
}

void svtkAbstractRenderDevice::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
