/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReaderAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReaderAlgorithm.h"

#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
svtkReaderAlgorithm::svtkReaderAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkReaderAlgorithm::~svtkReaderAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkReaderAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
