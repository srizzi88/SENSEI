/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTrivialConsumer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTrivialConsumer.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkTrivialConsumer);

//----------------------------------------------------------------------------
svtkTrivialConsumer::svtkTrivialConsumer()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
svtkTrivialConsumer::~svtkTrivialConsumer() = default;

//----------------------------------------------------------------------------
void svtkTrivialConsumer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkTrivialConsumer::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkTrivialConsumer::FillOutputPortInformation(int, svtkInformation*)
{
  return 1;
}
