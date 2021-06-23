/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLHierarchicalBoxDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLHierarchicalBoxDataReader.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkXMLHierarchicalBoxDataReader);

//----------------------------------------------------------------------------
svtkXMLHierarchicalBoxDataReader::svtkXMLHierarchicalBoxDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLHierarchicalBoxDataReader::~svtkXMLHierarchicalBoxDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLHierarchicalBoxDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
