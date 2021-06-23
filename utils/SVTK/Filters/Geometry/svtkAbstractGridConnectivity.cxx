/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAbstractGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAbstractGridConnectivity.h"

svtkAbstractGridConnectivity::svtkAbstractGridConnectivity()
{
  this->NumberOfGrids = 0;
  this->NumberOfGhostLayers = 0;
  this->AllocatedGhostDataStructures = false;
}

//------------------------------------------------------------------------------
svtkAbstractGridConnectivity::~svtkAbstractGridConnectivity()
{
  this->DeAllocateUserRegisterDataStructures();
  this->DeAllocateInternalDataStructures();
}

//------------------------------------------------------------------------------
void svtkAbstractGridConnectivity::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfGrids: " << this->NumberOfGrids << std::endl;
  os << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}
