/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericVertexAttributeMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericVertexAttributeMapping.h"

#include "svtkObjectFactory.h"
#include <string>
#include <vector>

#include <sstream>

class svtkGenericVertexAttributeMapping::svtkInternal
{
public:
  struct svtkInfo
  {
    std::string AttributeName;
    std::string ArrayName;
    int FieldAssociation;
    int Component;
    int TextureUnit;
  };

  typedef std::vector<svtkInfo> VectorType;
  VectorType Mappings;
};

svtkStandardNewMacro(svtkGenericVertexAttributeMapping);
//----------------------------------------------------------------------------
svtkGenericVertexAttributeMapping::svtkGenericVertexAttributeMapping()
{
  this->Internal = new svtkInternal();
}

//----------------------------------------------------------------------------
svtkGenericVertexAttributeMapping::~svtkGenericVertexAttributeMapping()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkGenericVertexAttributeMapping::AddMapping(
  const char* attributeName, const char* arrayName, int fieldAssociation, int component)
{
  if (!attributeName || !arrayName)
  {
    svtkErrorMacro("arrayName and attributeName cannot be null.");
    return;
  }

  if (this->RemoveMapping(attributeName))
  {
    svtkWarningMacro("Replacing existing mapping for attribute " << attributeName);
  }

  svtkInternal::svtkInfo info;
  info.AttributeName = attributeName;
  info.ArrayName = arrayName;
  info.FieldAssociation = fieldAssociation;
  info.Component = component;
  info.TextureUnit = -1;
  this->Internal->Mappings.push_back(info);
}

//----------------------------------------------------------------------------
void svtkGenericVertexAttributeMapping::AddMapping(
  int unit, const char* arrayName, int fieldAssociation, int component)
{
  std::ostringstream attributeName;
  attributeName << unit;

  if (this->RemoveMapping(attributeName.str().c_str()))
  {
    svtkWarningMacro("Replacing existing mapping for attribute " << attributeName.str().c_str());
  }

  svtkInternal::svtkInfo info;
  info.AttributeName = attributeName.str();
  info.ArrayName = arrayName;
  info.FieldAssociation = fieldAssociation;
  info.Component = component;
  info.TextureUnit = unit;
  this->Internal->Mappings.push_back(info);
}

//----------------------------------------------------------------------------
bool svtkGenericVertexAttributeMapping::RemoveMapping(const char* attributeName)
{
  svtkInternal::VectorType::iterator iter;
  for (iter = this->Internal->Mappings.begin(); iter != this->Internal->Mappings.end(); ++iter)
  {
    if (iter->AttributeName == attributeName)
    {
      this->Internal->Mappings.erase(iter);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkGenericVertexAttributeMapping::RemoveAllMappings()
{
  this->Internal->Mappings.clear();
}

//----------------------------------------------------------------------------
unsigned int svtkGenericVertexAttributeMapping::GetNumberOfMappings()
{
  return static_cast<unsigned int>(this->Internal->Mappings.size());
}

//----------------------------------------------------------------------------
const char* svtkGenericVertexAttributeMapping::GetAttributeName(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    svtkErrorMacro("Invalid index " << index);
    return nullptr;
  }
  return this->Internal->Mappings[index].AttributeName.c_str();
}

//----------------------------------------------------------------------------
const char* svtkGenericVertexAttributeMapping::GetArrayName(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    svtkErrorMacro("Invalid index " << index);
    return nullptr;
  }
  return this->Internal->Mappings[index].ArrayName.c_str();
}

//----------------------------------------------------------------------------
int svtkGenericVertexAttributeMapping::GetFieldAssociation(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    svtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].FieldAssociation;
}

//----------------------------------------------------------------------------
int svtkGenericVertexAttributeMapping::GetComponent(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    svtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].Component;
}

//----------------------------------------------------------------------------
int svtkGenericVertexAttributeMapping::GetTextureUnit(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    svtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].TextureUnit;
}

//----------------------------------------------------------------------------
void svtkGenericVertexAttributeMapping::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  svtkInternal::VectorType::iterator iter;
  for (iter = this->Internal->Mappings.begin(); iter != this->Internal->Mappings.end(); ++iter)
  {
    os << indent << "Mapping: " << iter->AttributeName.c_str() << ", " << iter->ArrayName.c_str()
       << ", " << iter->FieldAssociation << ", " << iter->Component << endl;
  }
}
