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
#include "svtkShaderProperty.h"

#include "svtkObjectFactory.h"
#include "svtkUniforms.h"
#include <algorithm>

svtkAbstractObjectFactoryNewMacro(svtkShaderProperty);

svtkShaderProperty::svtkShaderProperty()
{
  this->VertexShaderCode = nullptr;
  this->FragmentShaderCode = nullptr;
  this->GeometryShaderCode = nullptr;
}

svtkShaderProperty::~svtkShaderProperty()
{
  this->SetVertexShaderCode(nullptr);
  this->SetFragmentShaderCode(nullptr);
  this->SetGeometryShaderCode(nullptr);
}

void svtkShaderProperty::DeepCopy(svtkShaderProperty* p)
{
  this->SetVertexShaderCode(p->GetVertexShaderCode());
  this->SetFragmentShaderCode(p->GetFragmentShaderCode());
  this->SetGeometryShaderCode(p->GetGeometryShaderCode());
}

svtkMTimeType svtkShaderProperty::GetShaderMTime()
{
  svtkMTimeType fragUniformMTime = this->FragmentCustomUniforms->GetUniformListMTime();
  svtkMTimeType vertUniformMTime = this->VertexCustomUniforms->GetUniformListMTime();
  svtkMTimeType geomUniformMTime = this->GeometryCustomUniforms->GetUniformListMTime();
  return std::max({ this->GetMTime(), fragUniformMTime, vertUniformMTime, geomUniformMTime });
}

bool svtkShaderProperty::HasVertexShaderCode()
{
  return this->VertexShaderCode && *this->VertexShaderCode;
}

bool svtkShaderProperty::HasFragmentShaderCode()
{
  return this->FragmentShaderCode && *this->FragmentShaderCode;
}

bool svtkShaderProperty::HasGeometryShaderCode()
{
  return this->GeometryShaderCode && *this->GeometryShaderCode;
}

//-----------------------------------------------------------------------------
void svtkShaderProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
