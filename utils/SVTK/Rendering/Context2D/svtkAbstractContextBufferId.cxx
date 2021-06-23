/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractContextBufferId.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAbstractContextBufferId.h"
#include "svtkObjectFactory.h"

svtkAbstractObjectFactoryNewMacro(svtkAbstractContextBufferId);

// ----------------------------------------------------------------------------
svtkAbstractContextBufferId::svtkAbstractContextBufferId()
{
  this->Width = 0;
  this->Height = 0;
}

// ----------------------------------------------------------------------------
svtkAbstractContextBufferId::~svtkAbstractContextBufferId() = default;

// ----------------------------------------------------------------------------
void svtkAbstractContextBufferId::ReleaseGraphicsResources() {}

//-----------------------------------------------------------------------------
void svtkAbstractContextBufferId::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
