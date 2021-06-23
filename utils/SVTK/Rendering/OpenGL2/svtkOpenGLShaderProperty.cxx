/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShaderProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLShaderProperty.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLUniforms.h"
#include <algorithm>

svtkStandardNewMacro(svtkOpenGLShaderProperty);

svtkOpenGLShaderProperty::svtkOpenGLShaderProperty() = default;

svtkOpenGLShaderProperty::~svtkOpenGLShaderProperty() = default;

void svtkOpenGLShaderProperty::DeepCopy(svtkOpenGLShaderProperty* p)
{
  svtkShaderProperty::DeepCopy(p);
  this->UserShaderReplacements = p->UserShaderReplacements;
}

void svtkOpenGLShaderProperty::AddVertexShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    svtkShader::Vertex, originalValue, replaceFirst, replacementValue, replaceAll);
}

void svtkOpenGLShaderProperty::AddFragmentShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    svtkShader::Fragment, originalValue, replaceFirst, replacementValue, replaceAll);
}

void svtkOpenGLShaderProperty::AddGeometryShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    svtkShader::Geometry, originalValue, replaceFirst, replacementValue, replaceAll);
}

int svtkOpenGLShaderProperty::GetNumberOfShaderReplacements()
{
  return static_cast<int>(UserShaderReplacements.size());
}

std::string svtkOpenGLShaderProperty::GetNthShaderReplacementTypeAsString(svtkIdType index)
{
  if (index >= static_cast<svtkIdType>(this->UserShaderReplacements.size()))
  {
    svtkErrorMacro(<< "Trying to access out of bound shader replacement.");
    return std::string("");
  }
  ReplacementMap::iterator it = this->UserShaderReplacements.begin();
  std::advance(it, index);
  if (it->first.ShaderType == svtkShader::Vertex)
  {
    return std::string("Vertex");
  }
  else if (it->first.ShaderType == svtkShader::Fragment)
  {
    return std::string("Fragment");
  }
  else if (it->first.ShaderType == svtkShader::Geometry)
  {
    return std::string("Geometry");
  }
  return std::string("Unknown");
}

void svtkOpenGLShaderProperty::GetNthShaderReplacement(svtkIdType index, std::string& name,
  bool& replaceFirst, std::string& replacementValue, bool& replaceAll)
{
  if (index >= static_cast<svtkIdType>(this->UserShaderReplacements.size()))
  {
    svtkErrorMacro(<< "Trying to access out of bound shader replacement.");
  }
  ReplacementMap::iterator it = this->UserShaderReplacements.begin();
  std::advance(it, index);
  name = it->first.OriginalValue;
  replaceFirst = it->first.ReplaceFirst;
  replacementValue = it->second.Replacement;
  replaceAll = it->second.ReplaceAll;
}

void svtkOpenGLShaderProperty::ClearVertexShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(svtkShader::Vertex, originalValue, replaceFirst);
}

void svtkOpenGLShaderProperty::ClearFragmentShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(svtkShader::Fragment, originalValue, replaceFirst);
}

void svtkOpenGLShaderProperty::ClearGeometryShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(svtkShader::Geometry, originalValue, replaceFirst);
}

void svtkOpenGLShaderProperty::ClearAllVertexShaderReplacements()
{
  this->ClearAllShaderReplacements(svtkShader::Vertex);
}

void svtkOpenGLShaderProperty::ClearAllFragmentShaderReplacements()
{
  this->ClearAllShaderReplacements(svtkShader::Fragment);
}

void svtkOpenGLShaderProperty::ClearAllGeometryShaderReplacements()
{
  this->ClearAllShaderReplacements(svtkShader::Geometry);
}

//-----------------------------------------------------------------------------
void svtkOpenGLShaderProperty::ClearAllShaderReplacements()
{
  this->SetVertexShaderCode(nullptr);
  this->SetFragmentShaderCode(nullptr);
  this->SetGeometryShaderCode(nullptr);
  this->UserShaderReplacements.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLShaderProperty::AddShaderReplacement(
  svtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  svtkShader::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  svtkShader::ReplacementValue values;
  values.Replacement = replacementValue;
  values.ReplaceAll = replaceAll;

  this->UserShaderReplacements[spec] = values;
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkOpenGLShaderProperty::ClearShaderReplacement(
  svtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue, bool replaceFirst)
{
  svtkShader::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  typedef std::map<svtkShader::ReplacementSpec, svtkShader::ReplacementValue>::iterator RIter;
  RIter found = this->UserShaderReplacements.find(spec);
  if (found != this->UserShaderReplacements.end())
  {
    this->UserShaderReplacements.erase(found);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLShaderProperty::ClearAllShaderReplacements(svtkShader::Type shaderType)
{
  bool modified = false;
  // First clear all shader code
  if ((shaderType == svtkShader::Vertex) && this->VertexShaderCode)
  {
    this->SetVertexShaderCode(nullptr);
    modified = true;
  }
  else if ((shaderType == svtkShader::Fragment) && this->FragmentShaderCode)
  {
    this->SetFragmentShaderCode(nullptr);
    modified = true;
  }

  // Now clear custom tag replacements
  std::map<svtkShader::ReplacementSpec, svtkShader::ReplacementValue>::iterator rIter;
  for (rIter = this->UserShaderReplacements.begin(); rIter != this->UserShaderReplacements.end();)
  {
    if (rIter->first.ShaderType == shaderType)
    {
      this->UserShaderReplacements.erase(rIter++);
      modified = true;
    }
    else
    {
      ++rIter;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void svtkOpenGLShaderProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
