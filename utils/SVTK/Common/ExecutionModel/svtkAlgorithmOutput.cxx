/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAlgorithmOutput.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAlgorithmOutput.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkAlgorithmOutput);

//----------------------------------------------------------------------------
svtkAlgorithmOutput::svtkAlgorithmOutput()
{
  this->Producer = nullptr;
  this->Index = 0;
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput::~svtkAlgorithmOutput() = default;

//----------------------------------------------------------------------------
void svtkAlgorithmOutput::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Producer)
  {
    os << indent << "Producer: " << this->Producer << "\n";
  }
  else
  {
    os << indent << "Producer: (none)\n";
  }
  os << indent << "Index: " << this->Index << "\n";
}

//----------------------------------------------------------------------------
void svtkAlgorithmOutput::SetIndex(int index)
{
  this->Index = index;
}

//----------------------------------------------------------------------------
int svtkAlgorithmOutput::GetIndex()
{
  return this->Index;
}

//----------------------------------------------------------------------------
svtkAlgorithm* svtkAlgorithmOutput::GetProducer()
{
  return this->Producer;
}

//----------------------------------------------------------------------------
void svtkAlgorithmOutput::SetProducer(svtkAlgorithm* producer)
{
  this->Producer = producer;
}
