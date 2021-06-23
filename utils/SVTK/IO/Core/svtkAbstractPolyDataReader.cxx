/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION See Also
// svtkOBJReader svtkPLYReader svtkSTLReader
#include "svtkAbstractPolyDataReader.h"

svtkAbstractPolyDataReader::svtkAbstractPolyDataReader()
  : svtkPolyDataAlgorithm()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

svtkAbstractPolyDataReader::~svtkAbstractPolyDataReader()
{
  this->SetFileName(nullptr);
}

void svtkAbstractPolyDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NONE") << endl;
}
