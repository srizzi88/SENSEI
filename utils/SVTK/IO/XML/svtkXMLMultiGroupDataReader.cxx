/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLMultiGroupDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLMultiGroupDataReader.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkXMLMultiGroupDataReader);
//----------------------------------------------------------------------------
svtkXMLMultiGroupDataReader::svtkXMLMultiGroupDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLMultiGroupDataReader::~svtkXMLMultiGroupDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLMultiGroupDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
