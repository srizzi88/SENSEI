/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractParticleWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractParticleWriter.h"
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Construct with no start and end write methods or arguments.
svtkAbstractParticleWriter::svtkAbstractParticleWriter()
{
  this->TimeStep = 0;
  this->TimeValue = 0.0;
  this->FileName = nullptr;
  this->CollectiveIO = 0;
}
//----------------------------------------------------------------------------
svtkAbstractParticleWriter::~svtkAbstractParticleWriter()
{
  delete[] this->FileName;
  this->FileName = nullptr;
}
//----------------------------------------------------------------------------
void svtkAbstractParticleWriter::SetWriteModeToCollective()
{
  this->SetCollectiveIO(1);
}
//----------------------------------------------------------------------------
void svtkAbstractParticleWriter::SetWriteModeToIndependent()
{
  this->SetCollectiveIO(0);
}
//----------------------------------------------------------------------------
void svtkAbstractParticleWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "CollectiveIO: " << this->CollectiveIO << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NONE") << endl;
}
