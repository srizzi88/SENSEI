/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUInfo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGPUInfo.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkGPUInfo);

// ----------------------------------------------------------------------------
svtkGPUInfo::svtkGPUInfo()
{
  this->DedicatedVideoMemory = 0;
  this->DedicatedSystemMemory = 0;
  this->SharedSystemMemory = 0;
}

// ----------------------------------------------------------------------------
svtkGPUInfo::~svtkGPUInfo() = default;

// ----------------------------------------------------------------------------
void svtkGPUInfo::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dedicated Video Memory in bytes: " << this->DedicatedVideoMemory << endl;
  os << indent << "Dedicated System Memory in bytes: " << this->DedicatedSystemMemory << endl;
  os << indent << "Shared System Memory in bytes: " << this->SharedSystemMemory << endl;
}
