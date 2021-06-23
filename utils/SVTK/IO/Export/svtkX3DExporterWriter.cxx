/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporterWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkX3DExporterWriter.h"

#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
svtkX3DExporterWriter::svtkX3DExporterWriter()
{
  this->WriteToOutputString = 0;
  this->OutputString = nullptr;
  this->OutputStringLength = 0;
}

//----------------------------------------------------------------------------
svtkX3DExporterWriter::~svtkX3DExporterWriter()
{
  delete[] this->OutputString;
  this->OutputString = nullptr;
}

//----------------------------------------------------------------------------
void svtkX3DExporterWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "On" : "Off")
     << std::endl;
  os << indent << "OutputStringLength: " << this->OutputStringLength << std::endl;
  if (this->OutputString)
  {
    os << indent << "OutputString: " << this->OutputString << std::endl;
  }
}

//----------------------------------------------------------------------------
char* svtkX3DExporterWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;

  this->OutputString = nullptr;
  this->OutputStringLength = 0;

  return tmp;
}
