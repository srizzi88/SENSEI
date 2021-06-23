/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeInterface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayVolumeInterface.h"

#include "svtkObjectFactory.h"

svtkObjectFactoryNewMacro(svtkOSPRayVolumeInterface);

// ----------------------------------------------------------------------------
svtkOSPRayVolumeInterface::svtkOSPRayVolumeInterface() = default;

// ----------------------------------------------------------------------------
svtkOSPRayVolumeInterface::~svtkOSPRayVolumeInterface() = default;

// ----------------------------------------------------------------------------
void svtkOSPRayVolumeInterface::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void svtkOSPRayVolumeInterface::Render(svtkRenderer* svtkNotUsed(ren), svtkVolume* svtkNotUsed(vol))
{
  cerr << "Warning SVTK is not linked to OSPRay so can not VolumeRender with it" << endl;
}
